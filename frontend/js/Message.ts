import Asset from "./Asset";
import Order from "./Order";
import Side from "./Side";
import Trade from "./Trade";

enum MessageType {
  REGISTER = 0,
  ORDER = 1,
  CANCEL = 2,
  ERROR = 3,
}

interface IncomingMessage {
  type: MessageType | undefined;
  error: string | undefined;
  user_id: number | undefined;
  username: string | undefined;
  trades: Trade[] | undefined;
  unmatched_order: Order | undefined;
  order_id: number | undefined;
}

interface OutgoingMessage {
  type: MessageType | undefined;
  user_id: number | undefined;
  username: string | undefined;
  asset: Asset | undefined;
  side: Side | undefined;
  price: number | undefined;
  volume: number | undefined;
  order_id: number | undefined;
}

export { MessageType, IncomingMessage, OutgoingMessage };
