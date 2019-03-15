#include "liebi/bank.hpp"
#include "eosio/eosio.token.hpp"

namespace liebi {

    void bank::setagent(const uint8_t stake_duration, const vector<name> stake_accounts) {
        require_auth(_self);
        require_recipient(STAKE_CONTRACT);

        eosio_assert(stake_accounts.size() == STAKE_AGENT_SIZE, "invalid stake account size");

        stake_account_index stakeaccount(_self, _self.value);
        auto iter = stakeaccount.find(stake_duration);
        if( iter != stakeaccount.end()) {
            stakeaccount.erase(iter);
        }

        for (auto &item : stake_accounts) {
            eosio_assert(is_account(item), "invalid stake account");
        }

        stakeaccount.emplace(_self, [&](auto &a) {
            a.stake_duration = stake_duration;
            a.stake_agents = stake_accounts;
        });
    }

    void bank::stakecpu(const name stake_agent, const name receiver, const asset stake_cpu_quantity, const uint8_t stake_duration) {
        require_auth(STAKE_CONTRACT);

        eosio_assert(is_account(receiver), "invalid account receiver");
        eosio_assert(stake_duration >=1 && stake_duration <= 30, "invalid stake duration");

        eosio_assert(stake_cpu_quantity.symbol == EOS_SYMBOL, "invalid stake_cpu_quantity symbol");
        eosio_assert(stake_cpu_quantity.is_valid(), "invalid asset stake_cpu_quantity");
        eosio_assert(stake_cpu_quantity.amount > 0, "asset stake_cpu_quantity not positive");

        const asset balance = eosio::token::get_balance(EOS_TOKEN_CONTRACT, _self, EOS_SYMBOL.code());
        eosio_assert(balance.amount >= stake_cpu_quantity.amount, "not enough balance for stake");

        asset zero_net_quantity = ZERO_EOS_ASSET;

        stake_account_index stakeaccount(_self, _self.value);
        auto iter = stakeaccount.find(stake_duration);
        eosio_assert(iter != stakeaccount.end(), "stake account not exist");
        for(auto &item : iter->stake_agents) {
            if(item == stake_agent) {
                bool transfer = false;
                stakebw(stake_agent, receiver, zero_net_quantity, stake_cpu_quantity, transfer);

                return;
            }
        }

        eosio_assert(false, "stake account not exist");
    }

    void bank::unstakecpu(const name from, const name receiver, const asset unstake_cpu_quantity) {
        require_auth(STAKE_CONTRACT);

        eosio_assert(is_account(from), "invalid account from");
        eosio_assert(is_account(receiver), "invalid account receiver");

        eosio_assert(unstake_cpu_quantity.symbol == EOS_SYMBOL, "invalid unstake_cpu_quantity symbol");
        eosio_assert(unstake_cpu_quantity.is_valid(), "invalid asset unstake_cpu_quantity");
        eosio_assert(unstake_cpu_quantity.amount > 0, "asset unstake_cpu_quantity not positive");

        asset zero_net_quantity = ZERO_EOS_ASSET;
        unstakebw(from, receiver, zero_net_quantity, unstake_cpu_quantity);
    }

    void bank::refund(const name owner, const asset refund_quantity) {
        require_auth(STAKE_CONTRACT);

        eosio_assert(is_account(owner), "invalid account owner");

        refunds_table refunds_tbl(EOSIO_SYSTEM_CONTRACT, owner.value);
        auto req = refunds_tbl.find(owner.value);
        if(req != refunds_tbl.end()) {
            send_action_refund(owner);
        }

        name from = owner;
        name to = _self;
        asset quantity = refund_quantity;
        send_action_transfer(from, to, quantity, "refund");
    }

    void bank::stakebw(name from, name receiver,
                       asset stake_net_quantity,
                       asset stake_cpu_quantity, bool transfer) {
        name transfer_from = _self;
        name transfer_to = from;
        asset transfer_quantity = stake_cpu_quantity;
        send_action_transfer(transfer_from, transfer_to, transfer_quantity, "stake cpu");

        send_action_stake(from, receiver, stake_net_quantity, stake_cpu_quantity, transfer);
    }

    void bank::unstakebw(name from, name receiver, asset unstake_net_quantity, asset unstake_cpu_quantity) {
        send_action_unstake(from, receiver, unstake_net_quantity, unstake_cpu_quantity);
    }

    void bank::send_action_stake(name from, name receiver,
                                 asset stake_net_quantity,
                                 asset stake_cpu_quantity, bool transfer) {
        action(
                permission_level{from, "active"_n},
                EOSIO_SYSTEM_CONTRACT,
                "delegatebw"_n,
                std::make_tuple(from, receiver, stake_net_quantity, stake_cpu_quantity, transfer)
        ).send();
    }

    void bank::send_action_unstake(name from, name receiver, asset unstake_net_quantity, asset unstake_cpu_quantity) {
        action(
                permission_level{from, "active"_n},
                EOSIO_SYSTEM_CONTRACT,
                "undelegatebw"_n,
                std::make_tuple(from, receiver, unstake_net_quantity, unstake_cpu_quantity)
        ).send();
    }

    void bank::send_action_refund(name owner) {
        action(
                permission_level{owner, "active"_n},
                EOSIO_SYSTEM_CONTRACT,
                "refund"_n,
                std::make_tuple(owner)
        ).send();
    }

    void bank::send_action_transfer(name from, name to, asset quantity, string memo) {
        action(
                permission_level{from, "active"_n},
                EOS_TOKEN_CONTRACT,
                "transfer"_n,
                std::make_tuple(from, to, quantity, memo)
        ).send();
    }
} /// namespace liebi

extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (code == receiver && action == name("setagent").value) {
        execute_action(name(receiver), name(code), &liebi::bank::setagent);
    } else if (code == receiver && action == name("stakecpu").value) {
        execute_action(name(receiver), name(code), &liebi::bank::stakecpu);
    } else if (code == receiver && action == name("unstakecpu").value) {
        execute_action(name(receiver), name(code), &liebi::bank::unstakecpu);
    } else if (code == receiver && action == name("refund").value) {
        execute_action(name(receiver), name(code), &liebi::bank::refund);
    }
}
};