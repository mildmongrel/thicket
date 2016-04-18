#ifndef TESTDEFAULTS_H
#define TESTDEFAULTS_H

#include "Draft.h"
//#include "PackGenerator.h"

class TestDefaults
{
public:
    /*
    class PackGenerator : public ::PackGenerator
    {
    public:
        virtual std::vector<std::string> generate()
        {
            std::vector<std::string> vec;
            for( int i = 0; i < 15; ++i )
            {
                vec.push_back( std::string("card") + std::to_string(i) );
            }
            return vec;
        }
    };
    */

    static std::vector<std::string> generateCards( int cards )
    {
        std::vector<std::string> vec;
        for( int i = 0; i < cards; ++i )
        {
            vec.push_back( std::string("card") + std::to_string(i) );
        }
        return vec;
    }
    /*
    static inline std::vector<RoundParameters> getRoundParameters()
    {
        PackGeneratorSharedPtr packGen = PackGeneratorSharedPtr( new PackGenerator() );
        std::vector<RoundParameters> roundParams;
        roundParams.push_back( RoundParameters( packGen, RoundParameters::CLOCKWISE ) );
        roundParams.push_back( RoundParameters( packGen, RoundParameters::COUNTERCLOCKWISE ) );
        roundParams.push_back( RoundParameters( packGen, RoundParameters::CLOCKWISE ) );
        return roundParams;
    }
    */
    static inline std::vector<Draft<>::RoundConfiguration> getRoundConfigurations( int rounds, int chairs, int cardsPerPack, int roundTimeoutTicks = 30 )
    {
        std::vector<Draft<>::RoundConfiguration> roundConfigs;
        for( int round = 0; round < rounds; ++round )
        {
            Draft<>::RoundConfiguration roundConfig( std::string("round") + std::to_string(round) );
            roundConfig.setTimeoutTicks( roundTimeoutTicks );
            for( int chair = 0; chair < chairs; ++chair )
            {
                roundConfig.setPack( chair, std::string("pack") + std::to_string(chair), generateCards( cardsPerPack ) );
            }
            roundConfigs.push_back( roundConfig );
        }
        return roundConfigs;
    }
};

#endif
