/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2011 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

// mapnik
#include <mapnik/symbolizer_helpers.hpp>
#include <mapnik/label_collision_detector.hpp>
#include <mapnik/font_engine_freetype.hpp>
#include <mapnik/text/layout.hpp>
#include "agg_conv_clip_polyline.h"

namespace mapnik {

template <typename FaceManagerT, typename DetectorT>
text_symbolizer_helper<FaceManagerT, DetectorT>::text_symbolizer_helper(const text_symbolizer &sym, const Feature &feature, const proj_transform &prj_trans, unsigned width, unsigned height, double scale_factor, const CoordTransform &t, FaceManagerT &font_manager, DetectorT &detector, const box2d<double> &query_extent)
    : sym_(sym),
      feature_(feature),
      prj_trans_(prj_trans),
      t_(t),
      detector_(detector),
      writer_(sym.get_metawriter()),
      dims_(0, 0, width, height),
      query_extent_(query_extent),
      points_on_line_(false),
      placement_(sym_.get_placement_options()->get_placement_info(scale_factor)),
      finder_(feature, detector, dims_, placement_, font_manager, scale_factor)
{
    initialize_geometries();
    if (!geometries_to_process_.size()) return;
    finder_.next_position();
    initialize_points();
}

template <typename FaceManagerT, typename DetectorT>
placements_list const& text_symbolizer_helper<FaceManagerT, DetectorT>::get()
{
    if (point_placement_)
    {
        while (next_point_placement());
    }
    else
    {
        while (next_line_placement());
    }
    return finder_.placements();
}

template <typename FaceManagerT, typename DetectorT>
bool text_symbolizer_helper<FaceManagerT, DetectorT>::next_line_placement()
{
    while (!geometries_to_process_.empty())
    {
        if (geo_itr_ == geometries_to_process_.end())
        {
            //Just processed the last geometry. Try next placement.
            if (!finder_.next_position()) return false; //No more placements
            //Start again from begin of list
            geo_itr_ = geometries_to_process_.begin();
            continue; //Reexecute size check
        }

        typedef agg::conv_clip_polyline<geometry_type> clipped_geometry_type;
        typedef coord_transform<CoordTransform,clipped_geometry_type> path_type;

        clipped_geometry_type clipped(**geo_itr_);
        clipped.clip_box(query_extent_.minx(), query_extent_.miny(),
                         query_extent_.maxx(), query_extent_.maxy());
        path_type path(t_, clipped, prj_trans_);
        bool success;
        if (points_on_line_) {
            success = finder_.find_point_on_line_placements(path);
        } else {
            success = finder_.find_line_placements(path);
        }
        if (success)
        {
            //Found a placement
            geo_itr_ = geometries_to_process_.erase(geo_itr_);
#if 0
            if (writer_.first) writer_.first->add_text(
                finder_->get_results(), finder_->get_extents(),
                feature_, t_, writer_.second);
#endif
            return true;
        }
        //No placement for this geometry. Keep it in geometries_to_process_ for next try.
        geo_itr_++;
    }
    return false;
}

template <typename FaceManagerT, typename DetectorT>
bool text_symbolizer_helper<FaceManagerT, DetectorT>::next_point_placement()
{
    while (!points_.empty())
    {
        if (point_itr_ == points_.end())
        {
            //Just processed the last point. Try next placement.
            if (!finder_.next_position()) return false; //No more placements
            //Start again from begin of list
            point_itr_ = points_.begin();
            continue; //Reexecute size check
        }
        if (finder_.find_point_placement(*point_itr_))
        {
            //Found a placement
            point_itr_ = points_.erase(point_itr_);
#if 0
            if (writer_.first) writer_.first->add_text(
                finder_->get_results(), finder_->get_extents(),
                feature_, t_, writer_.second);
#endif
            return true;
        }
        //No placement for this point. Keep it in points_ for next try.
        point_itr_++;
    }
    return false;
}

struct largest_bbox_first
{
    bool operator() (geometry_type const* g0, geometry_type const* g1) const
    {
        box2d<double> b0 = g0->envelope();
        box2d<double> b1 = g1->envelope();
        return b0.width()*b0.height() > b1.width()*b1.height();
    }

};

template <typename FaceManagerT, typename DetectorT>
void text_symbolizer_helper<FaceManagerT, DetectorT>::initialize_geometries()
{
    bool largest_box_only = false;
    unsigned num_geom = feature_.num_geometries();
    for (unsigned i=0; i<num_geom; ++i)
    {
        geometry_type const& geom = feature_.get_geometry(i);

        // don't bother with empty geometries
        if (geom.size() == 0) continue;
        eGeomType type = geom.type();
        if (type == Polygon)
        {
            largest_box_only = sym_.largest_bbox_only();
            if (sym_.get_minimum_path_length() > 0)
            {
                box2d<double> gbox = t_.forward(geom.envelope(), prj_trans_);
                if (gbox.width() < sym_.get_minimum_path_length())
                {
                    continue;
                }
            }
        }
        // TODO - calculate length here as well
        geometries_to_process_.push_back(const_cast<geometry_type*>(&geom));
    }

    if (largest_box_only)
    {
        geometries_to_process_.sort(largest_bbox_first());
        geo_itr_ = geometries_to_process_.begin();
        geometries_to_process_.erase(++geo_itr_,geometries_to_process_.end());
    }
    geo_itr_ = geometries_to_process_.begin();
}

template <typename FaceManagerT, typename DetectorT>
void text_symbolizer_helper<FaceManagerT, DetectorT>::initialize_points()
{
    label_placement_enum how_placed = placement_->properties.label_placement;
    if (how_placed == LINE_PLACEMENT)
    {
        point_placement_ = false;
        return;
    }
    else
    {
        point_placement_ = true;
    }

    double label_x=0.0;
    double label_y=0.0;
    double z=0.0;

    std::list<geometry_type*>::const_iterator itr = geometries_to_process_.begin();
    std::list<geometry_type*>::const_iterator end = geometries_to_process_.end();
    for (; itr != end; itr++)
    {
        geometry_type const& geom = **itr;
        if (how_placed == VERTEX_PLACEMENT)
        {
            geom.rewind(0);
            for(unsigned i = 0; i < geom.size(); i++)
            {
                geom.vertex(&label_x, &label_y);
                prj_trans_.backward(label_x, label_y, z);
                t_.forward(&label_x, &label_y);
                points_.push_back(pixel_position(label_x, label_y));
            }
        }
        else
        {
            if (how_placed == POINT_PLACEMENT)
            {
                label::centroid(geom, label_x, label_y);
            }
            else if (how_placed == INTERIOR_PLACEMENT)
            {
                label::interior_position(geom, label_x, label_y);
            }
            else
            {
                MAPNIK_LOG_ERROR(symbolizer_helpers) << "ERROR: Unknown placement type in initialize_points()";
            }
            prj_trans_.backward(label_x, label_y, z);
            t_.forward(&label_x, &label_y);
            points_.push_back(pixel_position(label_x, label_y));
        }
    }
    point_itr_ = points_.begin();
}

/*****************************************************************************/


template <typename FaceManagerT, typename DetectorT>
shield_symbolizer_helper<FaceManagerT, DetectorT>::shield_symbolizer_helper(const shield_symbolizer &sym, const Feature &feature, const proj_transform &prj_trans, unsigned width, unsigned height, double scale_factor, const CoordTransform &t, FaceManagerT &font_manager, DetectorT &detector, const box2d<double> &query_extent)
    : text_symbolizer_helper<FaceManagerT, DetectorT>(
          sym, feature, prj_trans, width, height,
          scale_factor, t, font_manager, detector, query_extent),
    sym_(sym)
{
    this->points_on_line_ = true;
    init_marker();
}

template <typename FaceManagerT, typename DetectorT>
bool shield_symbolizer_helper<FaceManagerT, DetectorT>::next()
{
    if (!marker_) return false;
    if (point_placement_)
        return next_point_placement();
    else
        return next_line_placement();
}

template <typename FaceManagerT, typename DetectorT>
bool shield_symbolizer_helper<FaceManagerT, DetectorT>::next_point_placement()
{
    pixel_position const& shield_pos = sym_.get_shield_displacement();
    while (!points_.empty())
    {
        if (point_itr_ == points_.end())
        {
            //Just processed the last point. Try next placement.
            if (!finder_.next_position()) return false; //No more placements
            //Start again from begin of list
            point_itr_ = points_.begin();
            continue; //Reexecute size check
        }
        pixel_position const& text_disp = placement_->properties.displacement;
        pixel_position label_pos = *point_itr_ + shield_pos;

        if (!finder_.find_point_placement(label_pos))
        {
            //No placement for this point. Keep it in points_ for next try.
            point_itr_++;
            continue;
        }
        //Found a label placement but not necessarily also a marker placement
        // check to see if image overlaps anything too
        if (!sym_.get_unlock_image())
        {
            // center image at text center position
            // remove displacement from image label
            //TODO
#if 0
            double lx = p[0].center.x - text_disp.first;
            double ly = p[0].center.y - text_disp.second;
            marker_x_ = lx - 0.5 * marker_w_;
            marker_y_ = ly - 0.5 * marker_h_;
            marker_ext_.re_center(lx, ly);
#endif
        }
        else
        {  // center image at reference location
            marker_pos_ = label_pos - 0.5 * marker_size_;
            marker_ext_.re_center(label_pos.x, label_pos.y);
        }

        if (placement_->properties.allow_overlap || detector_.has_placement(marker_ext_))
        {
            detector_.insert(marker_ext_);
#if 0
            if (writer_.first) {
                writer_.first->add_box(marker_ext_, feature_, t_, writer_.second);
                writer_.first->add_text(finder_->get_results(), finder_->get_extents(),
                                        feature_, t_, writer_.second);
            }
#endif
            point_itr_ = points_.erase(point_itr_);
            return true;
        }
        //No placement found. Try again
        point_itr_++;
    }
    return false;
}


template <typename FaceManagerT, typename DetectorT>
bool shield_symbolizer_helper<FaceManagerT, DetectorT>::next_line_placement()
{
    pixel_position const& pos = placement_->properties.displacement;
#if 0
    finder_->additional_boxes.clear();
    //Markers are automatically centered
    finder_->additional_boxes.push_back(
        box2d<double>(-0.5 * marker_ext_.width()  - pos.first,
                      -0.5 * marker_ext_.height() - pos.second,
                      0.5 * marker_ext_.width()  - pos.first,
                      0.5 * marker_ext_.height() - pos.second));
#endif
    return text_symbolizer_helper<FaceManagerT, DetectorT>::next_line_placement();
}


template <typename FaceManagerT, typename DetectorT>
void shield_symbolizer_helper<FaceManagerT, DetectorT>::init_marker()
{
    std::string filename = path_processor_type::evaluate(*sym_.get_filename(), this->feature_);
    evaluate_transform(image_transform_, feature_, sym_.get_image_transform());
    marker_.reset();
    if (!filename.empty())
    {
        marker_ = marker_cache::instance()->find(filename, true);
    }
    if (!marker_) {
        marker_size_.clear();
        marker_ext_.init(0, 0, 0, 0);
        return;
    }
    marker_size_.set((*marker_)->width(), (*marker_)->height());
    double px0 = - 0.5 * marker_size_.x;
    double py0 = - 0.5 * marker_size_.y;
    double px1 = 0.5 * marker_size_.x;
    double py1 = 0.5 * marker_size_.y;
    double px2 = px1;
    double py2 = py0;
    double px3 = px0;
    double py3 = py1;
    image_transform_.transform(&px0,&py0);
    image_transform_.transform(&px1,&py1);
    image_transform_.transform(&px2,&py2);
    image_transform_.transform(&px3,&py3);
    marker_ext_.init(px0, py0, px1, py1);
    marker_ext_.expand_to_include(px2, py2);
    marker_ext_.expand_to_include(px3, py3);
}

template <typename FaceManagerT, typename DetectorT>
pixel_position shield_symbolizer_helper<FaceManagerT, DetectorT>::get_marker_position(glyph_positions_ptr p)
{
#if 0
    position const& pos = placement_->properties.displacement;
    if (placement_->properties.label_placement == LINE_PLACEMENT) {
        double lx = p.center.x - pos.first;
        double ly = p.center.y - pos.second;
        double px = lx - 0.5*marker_w_;
        double py = ly - 0.5*marker_h_;
        marker_ext_.re_center(lx, ly);
        //label is added to detector by get_line_placement(), but marker isn't
        detector_.insert(marker_ext_);
        if (writer_.first) writer_.first->add_box(marker_ext_, feature_, t_, writer_.second);
        return pixel_position(px, py);
    } else {
        //collision_detector is already updated for point placement in get_point_placement()
        return pixel_position(marker_x_, marker_y_);
    }
#else
#warning get_marker_position() disabled!
#endif
}


template <typename FaceManagerT, typename DetectorT>
marker& shield_symbolizer_helper<FaceManagerT, DetectorT>::get_marker() const
{
    return **marker_;
}

template <typename FaceManagerT, typename DetectorT>
agg::trans_affine const& shield_symbolizer_helper<FaceManagerT, DetectorT>::get_image_transform() const
{
    return image_transform_;
}

template class text_symbolizer_helper<face_manager<freetype_engine>, label_collision_detector4>;
template class shield_symbolizer_helper<face_manager<freetype_engine>, label_collision_detector4>;

} //namespace
