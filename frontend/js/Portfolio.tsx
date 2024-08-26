import React, { useContext } from "react";
import Asset from "./Asset";
import { GameStateContext } from "./App";

const Portfolio = () => {
  const { gameState } = useContext(GameStateContext);

  if (!gameState) {
    return <p>Loading...</p>;
  }

  const portfolioValue =
    gameState.cash +
    gameState.assets_held.reduce(
      (acc, q, i) => acc + q * Asset.value(i as Asset),
      0,
    ) +
    Math.min(...gameState.assets_held) * Asset.rueben_bonus;

  return (
    <>
      <h3>Portfolio: ${portfolioValue}</h3>
      <table>
        <thead>
          <tr>
            <th></th>
            <th>Cash</th>
            {Asset.assets.map((asset: Asset) => {
              return (
                <th key={asset}>
                  {Asset.toStringProper(asset)} (${Asset.value(asset)})
                </th>
              );
            })}
            <th>Rueben Bonus ($100)</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td>Amount Held</td>
            <td>${gameState.cash}</td>
            {Asset.assets.map((asset: Asset) => {
              return (
                <td key={asset + "held"}>
                  {gameState.assets_held[asset as number]}
                </td>
              );
            })}
            <td>{Math.min(...gameState.assets_held)}</td>
          </tr>
          <tr>
            <td>Value</td>
            <td>${gameState.cash}</td>
            {Asset.assets.map((asset: Asset) => {
              return (
                <td key={asset + "held"}>
                  ${gameState.assets_held[asset as number] * Asset.value(asset)}
                </td>
              );
            })}
            <td>${Math.min(...gameState.assets_held) * Asset.rueben_bonus}</td>
          </tr>
          <tr>
            <td>Buying/Selling Power</td>
            <td>${gameState.buying_power}</td>
            {Asset.assets.map((asset: Asset) => {
              return (
                <td key={asset + "held"}>
                  {gameState.selling_power[asset as number]}
                </td>
              );
            })}
            <td></td>
          </tr>
        </tbody>
      </table>
    </>
  );
};

export default Portfolio;
