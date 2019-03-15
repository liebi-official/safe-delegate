#pragma

#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>

#include "liebi/config.hpp"

using namespace std;
using namespace eosio;

namespace liebi {
    class [[eosio::contract("liebi.bank")]] bank : contract {
    public:
        using contract::contract;

        [[eosio::action]]
        void setagent(const uint8_t stake_duration, const vector<name> stake_accounts);

        [[eosio::action]]
        void stakecpu(const name stake_agent, const name receiver, const asset stake_cpu_quantity, const uint8_t stake_duration);

        [[eosio::action]]
        void unstakecpu(const name from, const name receiver, const asset unstake_cpu_quantity);

        [[eosio::action]]
        void refund(const name owner, const asset refund_quantity);
    private:
        void stakebw(name from, name receiver,
                     asset stake_net_quantity,
                     asset stake_cpu_quantity, bool transfer);

        void unstakebw(name from, name receiver, asset unstake_net_quantity, asset unstake_cpu_quantity);

        void send_action_stake(name from, name receiver,
                               asset stake_net_quantity,
                               asset stake_cpu_quantity, bool transfer);

        void send_action_unstake(name from, name receiver, asset unstake_net_quantity, asset unstake_cpu_quantity);

        void send_action_refund(name owner);

        void send_action_transfer(name from, name to, asset quantity, string memo);

        struct [[eosio::table]] stake_account {
            uint8_t stake_duration;
            vector<name> stake_agents;

            uint8_t primary_key() const { return stake_duration; }
            EOSLIB_SERIALIZE(stake_account, (stake_duration)(stake_agents));
        };


        struct refund_request {
            name            owner;
            time_point_sec  request_time;
            eosio::asset    net_amount;
            eosio::asset    cpu_amount;

            bool is_empty()const { return net_amount.amount == 0 && cpu_amount.amount == 0; }
            uint64_t  primary_key()const { return owner.value; }

            // explicit serialization macro is not necessary, used here only to improve compilation time
            EOSLIB_SERIALIZE( refund_request, (owner)(request_time)(net_amount)(cpu_amount) )
        };

        typedef eosio::multi_index<"stakeaccount"_n, stake_account> stake_account_index;

        typedef eosio::multi_index< "refunds"_n, refund_request > refunds_table;
    };
} /// namespace liebi
