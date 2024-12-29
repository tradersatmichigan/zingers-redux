#[derive(Debug)]
pub enum Asset {
    DRESSING = 0,
    RYE = 1,
    SWISS = 2,
    PASTRAMI = 3,
}

#[derive(Debug)]
pub enum Side {
    BUY = 0,
    SELL = 1,
}

#[derive(Debug)]
pub struct Trade {
    pub asset: Asset,
    pub buyer_id: u32,
    pub seller_id: u32,
    pub price: u32,
    pub volume: u32,
    pub id: u32,
}

#[derive(Debug)]
pub struct Order {
    pub asset: Asset,
    pub side: Side,
    pub user_id: u32,
    pub price: u32,
    pub volume: u32,
    pub order_id: u32,
}
