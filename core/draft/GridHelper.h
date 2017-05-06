#ifndef GRIDHELPER_H
#define GRIDHELPER_H

#include <set>
#include <map>

class GridHelper 
{
public:

    typedef std::set<unsigned int> IndexSet;

    const unsigned int ROW_COUNT = 3;
    const unsigned int COL_COUNT = 3;

    GridHelper() {}

    // A "slice" is a single index to specify rows and columns.
    unsigned int getRowSlice( unsigned int row ) const { return row; }
    unsigned int getColSlice( unsigned int col ) const { return col + ROW_COUNT; }
    unsigned int getSliceCount() const { return ROW_COUNT + COL_COUNT; }
    bool isSliceRow( unsigned int slice ) const { return slice < ROW_COUNT; }
    bool isSliceCol( unsigned int slice ) const { return (slice >= ROW_COUNT) && (slice < ROW_COUNT + COL_COUNT); }
    unsigned int getIndexCount() const { return ROW_COUNT * COL_COUNT; }

    // Get all available selections in the grid, mapped to the slice index to which it corresponds.
    std::map<IndexSet,unsigned int> getAvailableSelectionsMap( const IndexSet& unavailableIndices ) const;

private:

};

#endif
