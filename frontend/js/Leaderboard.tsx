import React, { useState, useEffect } from "react";
import UserInfo from "./UserInfo";

interface Leaderboard_t {
  [username: string]: number;
}

const Leaderboard = ({ userInfo }: { userInfo: UserInfo | undefined }) => {
  const [leaderboard, setLeaderboard] = useState<Leaderboard_t | null>(null);

  const fetchLeaderboard = async () => {
    fetch("/api/game/get_leaderboard")
      .then((response) => {
        if (!response.ok) throw Error(response.statusText);
        return response.json() as Promise<Leaderboard_t>;
      })
      .then((data: Leaderboard_t) => {
        setLeaderboard(data);
      })
      .catch((error) => console.error(error));
  };

  useEffect(() => {
    fetchLeaderboard();
    const intervalId = setInterval(fetchLeaderboard, 10000);

    return () => clearInterval(intervalId);
  }, []);

  return (
    <>
      <h3>Leaderboard</h3>
      {leaderboard ? (
        <table>
          <thead>
            <tr>
              <th style={{ textAlign: "left" }}>Rank</th>
              <th style={{ textAlign: "left" }}>Username</th>
              <th style={{ textAlign: "right" }}>Portfolio Value</th>
            </tr>
          </thead>
          <tbody>
            {Object.entries(leaderboard)
              .sort(([, lhs], [, rhs]) => rhs - lhs)
              .map(([username, value], index) => (
                <tr
                  key={username}
                  className={
                    userInfo?.username === username ? "user-row" : undefined
                  }
                >
                  <td style={{ textAlign: "left" }}>{index + 1}</td>
                  <td style={{ textAlign: "left" }}>{username}</td>
                  <td style={{ textAlign: "right" }}>${value}</td>
                </tr>
              ))}
          </tbody>
        </table>
      ) : (
        <p>Loading...</p>
      )}
    </>
  );
};

export default Leaderboard;
