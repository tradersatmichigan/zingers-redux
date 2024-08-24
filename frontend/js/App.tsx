import React, { createContext, useEffect, useState } from "react";
import UserInfo from "./UserInfo";
import GameState from "./GameState";
import AssetInterface from "./AssetInterface";
import Asset from "./Asset";
import PositionInterface from "./PositionInterface";
import Portfolio from "./Portfolio";
import Leaderboard from "./Leaderboard";

const GameStateContext = createContext<{
  gameState: GameState | undefined;
  setGameState:
  | React.Dispatch<React.SetStateAction<GameState | undefined>>
  | undefined;
}>({
  gameState: undefined,
  setGameState: undefined,
});

const ConnectionContext = createContext<{
  connections: { [key: number]: React.MutableRefObject<WebSocket | undefined> };
  setConnections:
  | React.Dispatch<
    React.SetStateAction<{
      [key: number]: React.MutableRefObject<WebSocket | undefined>;
    }>
  >
  | undefined;
}>({
  connections: {},
  setConnections: undefined,
});

const App = () => {
  const [userInfo, setUserInfo] = useState<UserInfo>();
  const [gameState, setGameState] = useState<GameState>();
  const [registered, setRegistered] = useState(
    Asset.assets.reduce(
      (acc, asset) => {
        acc[asset] = false;
        return acc;
      },
      {} as Record<Asset, boolean>,
    ),
  );

  const [connections, setConnections] = useState<{
    [key: number]: React.MutableRefObject<WebSocket | undefined>;
  }>({});

  useEffect(() => {
    fetch("/api/get_user_info/")
      .then((response) => {
        if (!response.ok) throw Error(response.statusText);
        return response.json() as Promise<UserInfo>;
      })
      .then((data: UserInfo) => {
        setUserInfo(data);
      })
      .catch((error) => console.error(error));
  }, []);

  useEffect(() => {
    if (!userInfo || !Object.values(registered).every((status) => status)) {
      return;
    }
    fetch("/api/game/get_state", {
      headers: {
        "User-Id": userInfo.user_id.toString(),
      },
    })
      .then((response) => {
        if (!response.ok) throw Error(response.statusText);
        return response.json() as Promise<GameState>;
      })
      .then((data: GameState) => {
        if (data.error) {
          throw Error(data.error);
        }
        setGameState(data);
      })
      .catch((error) => console.error(error));
  }, [userInfo, registered]);

  const handle_register_message = (asset: Asset) => {
    setRegistered((registered) => ({ ...registered, [asset]: true }));
  };

  return (
    <GameStateContext.Provider value={{ gameState, setGameState }}>
      <Leaderboard />
      <p>userInfo: {JSON.stringify(userInfo)}</p>
      <p>gameState: {JSON.stringify(gameState)}</p>
      <Portfolio />
      <ConnectionContext.Provider value={{ connections, setConnections }}>
        <PositionInterface userInfo={userInfo} />
        {Asset.assets.map((asset: Asset) => {
          return (
            <AssetInterface
              key={asset}
              asset={asset}
              userInfo={userInfo}
              handle_register_message={handle_register_message}
            />
          );
        })}
      </ConnectionContext.Provider>
    </GameStateContext.Provider>
  );
};

export { GameStateContext, ConnectionContext };
export default App;
