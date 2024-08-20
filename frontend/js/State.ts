import Order from "./Order";

interface StateResponse {
  error: string;
  orders: { [key: string]: Order };
  cash: number;
  buying_power: number;
  assets_held: number[];
  selling_power: number[];
}

export default StateResponse;
