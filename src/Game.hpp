#pragma once

#include <cassert>
#include <cstdint>
#include <mutex>
#include "Exchange.hpp"
#include "Types.hpp"

static std::array<Exchange, NUM_ASSETS> exchanges = {
    Exchange{DRESSING},
    Exchange{RYE},
    Exchange{SWISS},
    Exchange{PASTRAMI},
};

static auto get_game_state(user_t user_id) -> GameState {
  GameState state{};
  state.asset_values = asset_values;
  {
    std::lock_guard lg(cash_mutex);
    // error check
    if (!user_cash.contains(user_id)) {
      state.error = "User " + std::to_string(user_id) + " not found.";
      return state;
    }
    state.cash = user_cash.at(user_id);
  }
  for (uint8_t i = 0; i < NUM_ASSETS; ++i) {
    const auto& exchange = exchanges[i];
    // error check
    if (!exchange.user_assets.contains(user_id)) {
      state.error = "User " + std::to_string(user_id) + " not found on " +
                    to_string(exchange.asset) + " exchange.";
      return state;
    }
    state.assets[i] = exchange.user_assets.at(user_id);

    for (const auto& [_, orders] : exchange.bids) {
      for (const auto& order : orders) {
        state.orders[order.id] = order;
      }
    }

    for (const auto& [_, orders] : exchange.asks) {
      for (const auto& order : orders) {
        state.orders[order.id] = order;
      }
    }
  }
  return state;
}

static auto get_portfolio_value(user_t user_id) -> price_t {
  GameState state{};
  state.asset_values = asset_values;
  {
    std::lock_guard lg(cash_mutex);
    assert(user_cash.contains(user_id));
    state.cash = user_cash.at(user_id);
  }
  for (uint8_t i = 0; i < NUM_ASSETS; ++i) {
    const auto& exchange = exchanges[i];
    // error check
    if (!exchange.user_assets.contains(user_id)) {
      state.error = "User " + std::to_string(user_id) + " not found on " +
                    to_string(exchange.asset) + " exchange.";
      return state;
    }
    state.assets[i] = exchange.user_assets.at(user_id);
  }
}
