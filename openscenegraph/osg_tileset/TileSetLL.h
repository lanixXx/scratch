/*
   Copyright (C) 2014 Preet Desai (preet.desai@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef SCRATCH_TILESET_LL_H
#define SCRATCH_TILESET_LL_H

#include <memory>
#include <algorithm>
#include <osg/Camera>

#include <GeometryUtils.h>

template <typename T>
void SplitSets(std::vector<T> const &sorted_list_a,
               std::vector<T> const &sorted_list_b,
               std::vector<T> &list_diff_a,
               std::vector<T> &list_diff_b,
               std::vector<T> &list_xsec)
{
    list_diff_a.resize(sorted_list_a.size());
    list_diff_b.resize(sorted_list_b.size());
    list_xsec.resize(std::min(sorted_list_a.size(),
                              sorted_list_b.size()));

    typename std::vector<T>::iterator it;

    it = std::set_difference(sorted_list_a.begin(),
                             sorted_list_a.end(),
                             sorted_list_b.begin(),
                             sorted_list_b.end(),
                             list_diff_a.begin());
    list_diff_a.resize(it-list_diff_a.begin());

    it = std::set_difference(sorted_list_b.begin(),
                             sorted_list_b.end(),
                             sorted_list_a.begin(),
                             sorted_list_a.end(),
                             list_diff_b.begin());
    list_diff_b.resize(it-list_diff_b.begin());

    it = std::set_intersection(sorted_list_a.begin(),
                               sorted_list_a.end(),
                               sorted_list_b.begin(),
                               sorted_list_b.end(),
                               list_xsec.begin());
    list_xsec.resize(it-list_xsec.begin());
}

class TileLL
{
public:
    typedef uint64_t Id;

    // Root tile constructor
    TileLL(GeoBounds const &bounds,
           uint32_t x,
           uint32_t y) :
        id(getIdFromParentXY(nullptr,x,y)),
        level(0),
        x(x),
        y(y),
        bounds(bounds),
        parent(nullptr),
        clip(k_clip_NONE)

    {
        // empty
        tile_px_res=-10;
    }

    // Child tile constructor
    TileLL(TileLL * parent,
           uint32_t x,
           uint32_t y) :
        id(getIdFromParentXY(parent,x,y)),
        level(parent->level + 1),
        x(x),
        y(y),
        bounds(getBounds(parent,x,y)),
        parent(parent),
        clip(k_clip_NONE)
    {
        // empty
        tile_px_res=-10;
    }


    // unique id:
    // w  z  x      y
    // 00 FF FFFFFF FFFFFF
    // w: placeholder (not yet used)
    // z: tile level (8 bits)
    // x: tile x (24 bits)
    // y: tile y (24 bits)
    Id const id;

    // convenience
    uint8_t  const level;
    uint32_t const x;
    uint32_t const y;

    GeoBounds const bounds;

    // quadtree relationships
    TileLL * parent;
    std::unique_ptr<TileLL> tile_LT;
    std::unique_ptr<TileLL> tile_LB;
    std::unique_ptr<TileLL> tile_RB;
    std::unique_ptr<TileLL> tile_RT;

    // clip
    static const uint8_t k_clip_LT = 1 << 0;
    static const uint8_t k_clip_LB = 1 << 1;
    static const uint8_t k_clip_RB = 1 << 2;
    static const uint8_t k_clip_RT = 1 << 3;
    static const uint8_t k_clip_NONE = 0;
    static const uint8_t k_clip_ALL = 15;

    uint8_t clip;

    int64_t tile_px_res;

    // ============================= //

    static bool CompareLevelDescending(TileLL const * a,TileLL const * b)
    {
        return (a->level < b->level);
    }

    static Id GetIdFromLevelXY(uint8_t level,
                               uint32_t x,
                               uint32_t y)
    {
        uint64_t level64 = level;
        uint64_t x64 = x;
        uint64_t y64 = y;
        uint64_t tile_id = 0;
        tile_id |= (level64 << 48);
        tile_id |= (x64 << 24);
        tile_id |= y64;

        return tile_id;
    }

    static void GetLevelXYFromId(Id const id,
                                 uint8_t &level,
                                 uint32_t &x,
                                 uint32_t &y)
    {
        level = static_cast<uint8_t>(id >> 48);
        x = static_cast<uint32_t>((id >> 24) & 0xFFFFFF);
        y = static_cast<uint32_t>(id & 0xFFFFFF);
    }

private:   
    static uint64_t getIdFromParentXY(TileLL const * parent,
                                      uint32_t x,
                                      uint32_t y)
    {
        uint64_t level64 = (parent==nullptr) ? 0 : parent->level + 1;
        uint64_t x64 = x;
        uint64_t y64 = y;
        uint64_t tile_id = 0;
        tile_id |= (level64 << 48);
        tile_id |= (x64 << 24);
        tile_id |= y64;

        return tile_id;
    }

    static GeoBounds getBounds(TileLL const * p,
                               uint32_t x,
                               uint32_t y)
    {
        GeoBounds b;
        double const lon_width = (p->bounds.maxLon - p->bounds.minLon)*0.5;
        double const lat_width = (p->bounds.maxLat - p->bounds.minLat)*0.5;

        b.minLon = p->bounds.minLon + (lon_width * (x - p->x*2));
        b.maxLon = b.minLon + lon_width;

        b.minLat = p->bounds.minLat + (lat_width * (y - p->y*2));
        b.maxLat = b.minLat + lat_width;

        return b;
    }
};

class TileSetLL
{
public:

    // ============================================================= //

    TileSetLL(GeoBounds const &bounds,
              uint8_t min_level,
              uint8_t max_level,
              uint8_t num_root_tiles_x,
              uint8_t num_root_tiles_y) :
        m_bounds(bounds),
        m_min_level(min_level),
        m_max_level(max_level),
        m_num_root_tiles_x(num_root_tiles_x),
        m_num_root_tiles_y(num_root_tiles_y)
    {
        // empty
    }

    virtual ~TileSetLL()
    {
        // empty
    }

    GeoBounds const & GetBounds() const
    {
        return m_bounds;
    }

    // TODO maybe using uint8s for all this is a
    // bad idea because of all the <-> int casts
    uint8_t GetMinLevel() const
    {
        return m_min_level;
    }

    uint8_t GetMaxLevel() const
    {
        return m_max_level;
    }

    uint8_t GetNumRootTilesX() const
    {
        return m_num_root_tiles_x;
    }

    uint8_t GetNumRootTilesY() const
    {
        return m_num_root_tiles_y;
    }

    virtual TileLL const * GetTile(TileLL::Id id) const = 0;

    virtual void UpdateTileSet(osg::Camera const * cam,
                               std::vector<TileLL::Id> &list_tiles_add,
                               std::vector<TileLL::Id> &list_tiles_upd,
                               std::vector<TileLL::Id> &list_tiles_rem) = 0;

private:
    GeoBounds const m_bounds;
    uint8_t const m_min_level;
    uint8_t const m_max_level;
    uint8_t const m_num_root_tiles_x;
    uint8_t const m_num_root_tiles_y;

    // ============================================================= //
};

#endif // SCRATCH_TILESET_LL_H
