# OpenTTD + RTS War Strategy Prototype

This repository tracks a prototype that layers a lightweight real-time strategy layer onto the OpenTTD engine.

## Goals
- preserve the transport and economy simulation core
- add military assets, orders, and combat behavior
- prototype RTS-style logistics and frontline pressure
- keep the system modular and readable so it can evolve without a full rewrite

## Current Prototype
- formation and squad movement
- supply pressure and return-to-base logic
- route planning and movement pressure
- tactical states: advance, hold, retreat
- demo battle scenario and frontline pressure model
- baseline AI objective selection

## Build
This project is built from the OpenTTD source tree and uses the normal CMake/MSBuild flow on Windows.

## Status
This is an active prototype and not a finished RTS replacement.
