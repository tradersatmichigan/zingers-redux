use crate::types::{Asset, Order, Side, Trade};
use hashlink::LinkedHashMap;
use std::collections::BTreeMap;

const MIN_PRICE: u32 = 1;
const MAX_PRICE: u32 = 20000;
const MIN_VOLUME: u32 = 1;
const MAX_VOLUME: u32 = 20000;

#[derive(Debug)]
pub struct Exchange {
    pub asset: Asset,
    bids: BTreeMap<u32, LinkedHashMap<u32, Order>>,
    asks: BTreeMap<u32, LinkedHashMap<u32, Order>>,
}

impl Exchange {
    pub fn place_order(&self, order: &Order) {}
    pub fn cancel_order(&self, order_id: &u32) {}
}
