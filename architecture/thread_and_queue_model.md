# MarketCore — Thread & Queue Model

Current architecture of the `market_engine_exe` process: `clsWSBNConnector` (Binance
WebSocket adapter) and `clsDataEngine` (order-book consumer), connected via
`clsFeedCommunicator`'s four SPSC queues.

## Thread topology

```mermaid
flowchart TB
    subgraph MAIN["main() - MarketEngine/main.cpp"]
        M1["Create clsFeedCommunicator, clsWSBNConnector, clsDataEngine"]
    end

    subgraph CONNECTOR["clsWSBNConnector::Init spawns 4 threads"]
        T1["SnapshotThreadLoop: pop SnapshotCommandQ, call SubscribeSnap"]
        T2["SnapshotReaderThread: blocking WS read, parse, push SnapshotDataQ"]
        T3["DepthThreadLoop: pop DepthCommandQ, call SubscribeDepth"]
        T4["DepthReaderThread: blocking WS read, parse, push DepthDataQ"]
    end

    subgraph ENGINE["clsDataEngine - 2 threads"]
        T5["SendDemoRequest thread: push SNAPSHOT and SUBSCRIBE commands, throttled"]
        T6["Run main thread: poll SnapshotDataQ and DepthDataQ, apply to per-symbol clsOrderBook, print"]
    end

    M1 --> T1
    M1 --> T2
    M1 --> T3
    M1 --> T4
    M1 --> T5
    M1 --> T6
```

**Thread ownership rules:**
- `clsWSBNConnector` owns 4 threads: two "command loop" threads that dequeue subscribe
  commands and issue the actual Binance WS requests, and two "reader" threads that
  block on `websocket::read` and push parsed messages into the data queues.
- `clsDataEngine` runs the demo subscription pusher (`SendDemoRequest`) on its own
  thread, while `Run()` (the order-book consumer loop) owns the main thread.
- No thread ever touches another thread's queue as anything but producer or consumer
  — see queue table below for the exact pairing.

## Queue topology (`clsFeedCommunicator`)

```mermaid
flowchart LR
    subgraph Engine["clsDataEngine"]
        SDR["SendDemoRequest thread"]
        RUN["Run() thread"]
    end

    subgraph Comm["clsFeedCommunicator (SPSC queues)"]
        SCQ[["SnapshotCommandQ"]]
        DCQ[["DepthCommandQ"]]
        SDQ[["SnapshotDataQ"]]
        DDQ[["DepthDataQ"]]
    end

    subgraph Connector["clsWSBNConnector"]
        SNAPLOOP["SnapshotThreadLoop"]
        DEPTHLOOP["DepthThreadLoop"]
        SNAPREAD["SnapshotReaderThread"]
        DEPTHREAD["DepthReaderThread"]
    end

    SDR -- "PushSnapShotCmd" --> SCQ
    SDR -- "PushDepthCmd" --> DCQ
    SCQ -- "PopSnapShotCmd" --> SNAPLOOP
    DCQ -- "PopDepthCmd" --> DEPTHLOOP

    BinanceSnapWS("Binance ws-api.binance.com")
    BinanceDepthWS("Binance stream.binance.com")
    OB["Symbol to OrderBook map"]

    SNAPLOOP -- "SubscribeSnap" --> BinanceSnapWS
    DEPTHLOOP -- "SubscribeDepth" --> BinanceDepthWS

    BinanceSnapWS -- "snapshot response" --> SNAPREAD
    BinanceDepthWS -- "depthUpdate" --> DEPTHREAD

    SNAPREAD -- "PushSnapshotData" --> SDQ
    DEPTHREAD -- "PushDepthData" --> DDQ

    SDQ -- "PopSnapshotData" --> RUN
    DDQ -- "PopDepthData" --> RUN

    RUN --> OB
```

**Queue rules (each is SPSC — single producer, single consumer):**

| Queue | Producer | Consumer | Carries |
|---|---|---|---|
| `SnapshotCommandQ` | `clsDataEngine::SendDemoRequest` | `clsWSBNConnector::SnapshotThreadLoop` | `stFeedCommand` (SNAPSHOT + requestId) |
| `DepthCommandQ` | `clsDataEngine::SendDemoRequest` | `clsWSBNConnector::DepthThreadLoop` | `stFeedCommand` (SUBSCRIBE) |
| `SnapshotDataQ` | `clsWSBNConnector::SnapshotReaderThread` | `clsDataEngine::Run` | `stMarketDataMessage` (SNAPSHOT) |
| `DepthDataQ` | `clsWSBNConnector::DepthReaderThread` | `clsDataEngine::Run` | `stMarketDataMessage` (INCREMENTAL_DEPTH_UPDATE) |

## Symbol correlation (snapshot has no symbol field)

```mermaid
sequenceDiagram
    participant SDR as SendDemoRequest thread
    participant SCQ as SnapshotCommandQ
    participant SNAPLOOP as SnapshotThreadLoop
    participant BIN as Binance ws-api
    participant SNAPREAD as SnapshotReaderThread
    participant SDQ as SnapshotDataQ
    participant RUN as Run() / clsDataEngine

    SDR->>SDR: assign requestId, remember requestId to symbol
    SDR->>SCQ: push SNAPSHOT command with symbol and requestId
    SCQ->>SNAPLOOP: pop cmd
    SNAPLOOP->>BIN: send request with id equal to requestId
    BIN-->>SNAPREAD: response with id, no symbol field
    SNAPREAD->>SNAPREAD: parse response, requestId set, symbol still empty
    SNAPREAD->>SDQ: push message
    SDQ->>RUN: pop message
    RUN->>RUN: look up symbol via requestId, set message symbol
    RUN->>RUN: apply snapshot to order book for that symbol
```

## Notes / known gaps
- `m_mpSymToRequestId` in `clsDataEngine` is currently unguarded by a mutex even
  though it's written by `SendDemoRequest` thread and read by `Run()` thread — see
  `/memories/repo/cmake.md` and session notes; revisit if this becomes a real race
  in practice (map only grows via `SendDemoRequest`, is read-only afterward per key).
- No sync/bridging state machine yet (buffering deltas until snapshot arrives, gap
  detection, resync) — incremental updates are applied directly to whatever book
  exists in the map; if depth arrives before the matching snapshot, it's dropped
  with a "Symbol not found" log until the snapshot creates the map entry.
