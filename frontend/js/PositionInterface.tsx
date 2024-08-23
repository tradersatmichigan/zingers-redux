import React, { useContext } from "react";
import { GameStateContext } from "./App";
import UserInfo from "./UserInfo";
import Side from "./Side";
import PositionTable from "./PositionTable";

const PositionInterface = ({
  userInfo,
}: {
  userInfo: UserInfo | undefined;
}) => {
  const { gameState } = useContext(GameStateContext);

  if (!gameState) {
    return <p>No orders</p>;
  }

  const buy_orders = Object.values(gameState.orders).filter(
    (order) => order.side === Side.BUY && order.user_id === userInfo?.user_id,
  );
  const sell_orders = Object.values(gameState.orders).filter(
    (order) => order.side === Side.SELL && order.user_id === userInfo?.user_id,
  );

  return (
    <>
      <PositionTable side={Side.BUY} orders={buy_orders} />
      <PositionTable side={Side.SELL} orders={sell_orders} />
    </>
  );
};

export default PositionInterface;
