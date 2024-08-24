import React, { useState, useEffect } from "react";

interface Leaderboard_t {
  [username: string]: number;
}

const Leaderboard = () => {
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
    <div>
      <h2>Leaderboard</h2>
      {leaderboard ? (
        <table>
          <thead>
            <tr>
              <th>Rank</th>
              <th>Username</th>
              <th>Portfolio Value</th>
            </tr>
          </thead>
          <tbody>
            {Object.entries(leaderboard)
              .sort(([, lhs], [, rhs]) => rhs - lhs)
              .map(([username, value], index) => (
                <tr key={username}>
                  <td>{index + 1}</td>
                  <td>{username}</td>
                  <td>{value}</td>
                </tr>
              ))}
          </tbody>
        </table>
      ) : (
        <p>Loading...</p>
      )}
    </div>
  );
};

export default Leaderboard;
