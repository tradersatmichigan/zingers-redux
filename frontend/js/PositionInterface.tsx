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
    return <p>Loading...</p>;
  }

  const buy_orders = Object.values(gameState.orders).filter(
    (order) => order.side === Side.BUY && order.user_id === userInfo?.user_id,
  );
  const sell_orders = Object.values(gameState.orders).filter(
    (order) => order.side === Side.SELL && order.user_id === userInfo?.user_id,
  );

  return (
    <>
      <h3>Positions</h3>
      <div
        style={{
          display: "flex",
          flexWrap: "wrap",
        }}
      >
        <div style={{ width: "50%" }}>
          <PositionTable side={Side.BUY} orders={buy_orders} />
        </div>
        <div style={{ width: "50%" }}>
          <PositionTable side={Side.SELL} orders={sell_orders} />
        </div>
      </div>
    </>
  );
};

export default PositionInterface;
