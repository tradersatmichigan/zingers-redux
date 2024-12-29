use crate::exchange::Exchange;
use crate::types::{Asset, Order, Side, Trade};

pub mod exchange;
pub mod types;

fn main() {
    let asset = Asset::DRESSING;
    let side = Side::BUY;
    let trade = Trade {
        asset: Asset::DRESSING,
        buyer_id: 0,
        seller_id: 0,
        price: 0,
        volume: 0,
        id: 0,
    };
    let order = Order {
        asset: Asset::DRESSING,
        side: Side::BUY,
        user_id: 0,
        price: 10,
        volume: 1,
        order_id: 0,
    };
    let exchange = Exchange {
        asset: Asset::DRESSING,
    };
    exchange.place_order(&order);
    println!("I'm have {asset:?}!");
    println!("I'm placing a {side:?} order!");
    println!("I'm completing a trade {trade:?}!");
    println!("I'm placing an order {order:?}!");
    println!("I have an exchange {exchange:?}!");
}
