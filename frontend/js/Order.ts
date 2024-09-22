import Asset from "./Asset";
import Side from "./Side";

type Order = {
  asset: Asset;
  side: Side;
  user_id: number;
  price: number;
  volume: number;
  order_id: number;
};

export default Order;
