import Order from "./Order";

interface GameState {
  error: string;
  orders: { [key: string]: Order };
  cash: number;
  buying_power: number;
  assets_held: number[];
  selling_power: number[];
}

export default GameState;
