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

    subgraph CONNECTOR["clsWSBNConnector::Init() spawns 4 threads"]
        T1["SnapshotThreadLoop\n(pop SnapshotCommandQ, call SubscribeSnap)"]
        T2["SnapshotReaderThread\n(blocking WS read, parse, push SnapshotDataQ)"]
        T3["DepthThreadLoop\n(pop DepthCommandQ, call SubscribeDepth)"]
        T4["DepthReaderThread\n(blocking WS read, parse, push DepthDataQ)"]
    end

    subgraph ENGINE["clsDataEngine - 2 threads"]
        T5["SendDemoRequest thread (t1 in main)\n(push SNAPSHOT+SUBSCRIBE commands, throttled)"]
        T6["Run() - main thread\n(poll SnapshotDataQ + DepthDataQ,\napply to per-symbol clsOrderBook, print)"]
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

    SNAPLOOP -- "SubscribeSnap()" --> BinanceSnapWS[("Binance ws-api.binance.com")]
    DEPTHLOOP -- "SubscribeDepth()" --> BinanceDepthWS[("Binance stream.binance.com")]

    BinanceSnapWS -- "snapshot response" --> SNAPREAD
    BinanceDepthWS -- "depthUpdate" --> DEPTHREAD

    SNAPREAD -- "PushSnapshotData" --> SDQ
    DEPTHREAD -- "PushDepthData" --> DDQ

    SDQ -- "PopSnapshotData" --> RUN
    DDQ -- "PopDepthData" --> RUN

    RUN --> OB[("map&lt;symbol, clsOrderBook&gt;")]
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

    SDR->>SDR: requestId = m_nextRequestId++<br/>m_mpSymToRequestId[requestId] = symbol
    SDR->>SCQ: push stFeedCommand{SNAPSHOT, symbol, requestId}
    SCQ->>SNAPLOOP: pop cmd
    SNAPLOOP->>BIN: {"id": requestId, "method":"depth", ...}
    BIN-->>SNAPREAD: {"id": requestId, "result": {...no symbol...}}
    SNAPREAD->>SNAPREAD: parse -> msg.requestId = id (symbol still empty)
    SNAPREAD->>SDQ: push msg
    SDQ->>RUN: pop msg
    RUN->>RUN: symbol = m_mpSymToRequestId[msg.requestId]<br/>msg.symbol = symbol
    RUN->>RUN: m_mporderSymToOrderBook[symbol].ApplySnapshot(msg)
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
