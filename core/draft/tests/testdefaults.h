#ifndef TESTDEFAULTS_H
#define TESTDEFAULTS_H

#include "Draft.h"
#include "DraftConfig.pb.h"

class TestDefaults
{
public:

    class TestCardDispenser : public DraftCardDispenser<std::string>
    {
    public:
        TestCardDispenser( const std::string& setCode = std::string() ) : mSetCode( setCode ) {}

        std::vector<std::string> dispense()
        {
            std::vector<std::string> vec;
            for( int i = 0; i < 15; ++i )
            {
                vec.push_back( mSetCode + std::string(":card") + std::to_string(i) );
            }
            return vec;
        }
    private:
        const std::string mSetCode;
    };

    static inline DraftCardDispenserSharedPtrVector<> getDispensers()
    {
        auto dispSptr = std::make_shared<TestCardDispenser>();
        DraftCardDispenserSharedPtrVector<> dispensers;
        dispensers.push_back( dispSptr );
        return dispensers;
    }

    static inline DraftConfig getDraftConfig( int rounds, int chairs, int roundSelectionTime = 30)
    {
        DraftConfig dc;
        dc.set_version( 1 );
        dc.set_chair_count( chairs );
        DraftConfig::CardDispenser* dispenser = dc.add_card_dispensers();
        dispenser->set_set_code( "" );
        dispenser->set_method( DraftConfig::CardDispenser::METHOD_BOOSTER );
        dispenser->set_replacement( DraftConfig::CardDispenser::REPLACEMENT_ALWAYS );
        for( int r = 0; r < rounds; ++r )
        {
            DraftConfig::Round* round = dc.add_rounds();
            DraftConfig::BoosterRound* boosterRound = round->mutable_booster_round();
            boosterRound->set_selection_time( roundSelectionTime );
            boosterRound->set_pass_direction( (r%2) == 0 ?
                    DraftConfig::DIRECTION_CLOCKWISE :
                    DraftConfig::DIRECTION_COUNTER_CLOCKWISE );
            DraftConfig::CardDispensation* dispensation = boosterRound->add_dispensations();
            dispensation->set_card_dispenser_index( 0 );
            for( int i = 0; i < chairs; ++i )
            {
                dispensation->add_chair_indices( i );
            }
        }
        return dc;
    }

};

#endif
