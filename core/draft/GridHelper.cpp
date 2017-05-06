#include "GridHelper.h"

#include <vector>
#include <algorithm>
#include <iterator>

static std::vector< GridHelper::IndexSet > sGridSlices = { { 0, 1, 2 }, // row 0
                                                           { 3, 4, 5 }, // row 1
                                                           { 6, 7, 8 }, // row 2
                                                           { 0, 3, 6 }, // col 0
                                                           { 1, 4, 7 }, // col 1
                                                           { 2, 5, 8 }  // col 2
                                                         };

std::map<GridHelper::IndexSet,unsigned int>
GridHelper::getAvailableSelectionsMap( const IndexSet& unavailableIndices ) const
{
    std::map<IndexSet,unsigned int> selectionsMap;

    for( unsigned int i = 0; i < sGridSlices.size(); ++i )
    {
        // Selection is valid if it contains exactly all indices in a row that are available.
        IndexSet indices;
        std::set_difference( sGridSlices[i].begin(), sGridSlices[i].end(),
                             unavailableIndices.begin(), unavailableIndices.end(),
                             std::inserter( indices, indices.begin() ) );

        if( !indices.empty() )
        {
            selectionsMap.insert( std::make_pair( indices, i ) );
        }
    }

    return selectionsMap;
}
