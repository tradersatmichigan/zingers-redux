enum Asset {
  DRESSING = 0,
  RYE = 1,
  SWISS = 2,
  PASTRAMI = 3,
}

namespace Asset {
  export const toString = (asset: Asset) => {
    switch (asset) {
      case Asset.DRESSING:
        return "dressing";
      case Asset.RYE:
        return "rye";
      case Asset.SWISS:
        return "swiss";
      case Asset.PASTRAMI:
        return "pastrami";
    }
  };

  export const toStringProper = (asset: Asset) => {
    switch (asset) {
      case Asset.DRESSING:
        return "Dressing";
      case Asset.RYE:
        return "Rye";
      case Asset.SWISS:
        return "Swiss";
      case Asset.PASTRAMI:
        return "Pastrami";
    }
  };

  export const abbreviate = (asset: Asset) => {
    switch (asset) {
      case Asset.DRESSING:
        return "DRS";
      case Asset.RYE:
        return "RYE";
      case Asset.SWISS:
        return "SWS";
      case Asset.PASTRAMI:
        return "PAS";
    }
  };

  export const value = (asset: Asset) => {
    switch (asset) {
      case Asset.DRESSING:
        return 10;
      case Asset.RYE:
        return 20;
      case Asset.SWISS:
        return 30;
      case Asset.PASTRAMI:
        return 40;
    }
  };

  export const assets = [
    Asset.DRESSING,
    Asset.RYE,
    Asset.SWISS,
    Asset.PASTRAMI,
  ];

  export const rueben_bonus = 100;
}

export default Asset;
