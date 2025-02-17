# Zero Lookahead? Zero Problem. The Window Racer Algorithm

![Window Racer animation](animation.gif)

This is an implementation of the Window Racer algorithm for optimistic parallel discrete-event simulation.
The key idea is to employ fine-grained locking to allow threads to follow chains of dependent events across thread boundaries to limit synchronization overheads.

As an example model, the repository contains the PHold benchmark model extended by events with near-zero delay.

## Quickstart

```
cmake . -DCMAKE_BUILD_TYPE=Release
./window_racer 1
```

## Associated publication (received best paper award at ACM SIGSIM PADS 2023):

*Philipp Andelfinger, Till Köster, and Adelinde Uhrmacher. 2023. Zero Lookahead? Zero Problem. The Window Racer Algorithm. In Proceedings of the 2023 ACM SIGSIM Conference on Principles of Advanced Discrete Simulation (SIGSIM-PADS '23). Association for Computing Machinery, New York, NY, USA, 1–11. https://doi.org/10.1145/3573900.3591115*
