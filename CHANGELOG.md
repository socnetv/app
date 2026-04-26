# Changelog

All notable changes to this project are documented in this file. 

## [3.5] – April 2026

### New Features

  - **Graph exploration filters** (WS9):
    - Focus on Node (Ego Network): hides all nodes except the selected node
      and its direct neighbors, and all non-incident edges. Available in the
      Filter menu and node right-click context menu (#211).
    - Focus on Selection: hides all nodes not in the current selection and all
      edges whose endpoints are not both selected. Action `Ctrl+X, Ctrl+S`;
      available in Filter menu and node right-click context menu (#210).
    - Restore All Nodes: restores all nodes hidden by any filter. Available in
      Filter menu and node right-click context menu.
    - Restore All Edges: re-enables all edges hidden by the weight filter.
      Action `Ctrl+E, Ctrl+R`; available in the Filter menu (#213).
    - All node-visibility filters (ego network, selection, centrality) now
      share a unified non-destructive snapshot/restore history stack — Restore
      All works across all filter types (#216).

  - **Ego-centered radial layout**: places a selected node at the canvas
    center, its 1-hop out-neighbors on an inner ring, and all remaining nodes
    on an outer ring. Available via Layout menu (`Ctrl+Alt+E`) and node
    right-click context menu (#214).

  - **Node/edge attribute system** (#224):
    - Single-key node attribute API: `Graph::vertexCustomAttributeSet()` /
      `vertexCustomAttributeRemove()`.
    - Edge custom attribute storage: `GraphVertex::m_outEdgeCustomAttributes`,
      `Graph::edgeCustomAttributesSet()` / `edgeCustomAttributes()`.
    - Edge Properties dialog (`DialogEdgeEdit`): edit label, weight, color and
      arbitrary custom key/value pairs; accessible from the toolbar and edge
      right-click context menu.
    - GraphML roundtrip for edge custom attributes: unique keys exported as
      `d2000+` `<key for="edge">` definitions; per-edge `<data>` tags written
      on save and parsed back on load.
    - Filter Nodes By Attribute: `Graph::vertexFilterByAttribute(key, value)` —
      non-destructive snapshot/restore filter; available in the Filter menu
      (`Ctrl+X, Ctrl+A`).
  - **Attribute-based filtering** (#217):
    - `FilterCondition` struct: scope (Nodes/Edges/Both), key, operator
      (`=` `≠` `>` `<` `≥` `≤` `contains`), value; `label()` for future
      filter bar chips.
    - `DialogFilterByAttribute`: scope selector, editable key combo populated
      from the graph's existing node/edge attribute keys, operator dropdown,
      free-text value field.
    - `Graph::vertexFilterByAttribute(FilterCondition)`: refactored to accept
      the full condition struct; numeric-aware comparison (tries `toDouble()`,
      falls back to lexicographic).
    - `Graph::edgeFilterByAttribute(FilterCondition)`: hides edges not
      matching the condition; uses the same snapshot/restore stack as node
      filters.
    - Filter combo added to the Control Panel (Network group) for one-click
      access to all filter actions.
    - Dedicated toolbar filter group with distinct icons for each filter action.

### Improvements

  - Force-directed layouts improved for large graphs:
    - Fruchterman-Reingold: pre-cached adjacency (O(1) edge lookup in inner
      loop), initial random placement, early convergence detection.
    - Kamada-Kawai: canvas clamping replaces random teleport on out-of-bounds
      particles.

### Bug Fixes

  - Fixed Kamada-Kawai crash when node filters are active.
  - Fixed crash on graph reset: guard edge creation in `setEdgeVisibility`
    when the edge has already been removed (#231).
  - Fixed visibility history stack not cleared on graph clear / `initApp`.
  - Fixed custom node attribute key/id mismatch in GraphML export (#208).
  - Fixed Pajek parser: use default node shape as fallback when no Pajek
    shape keyword is present (#179).
  - Fixed `DialogClusteringHierarchical` signal/slot mismatch (#194).
  - Fixed Node Properties dialog UX issues for custom attributes (#130).
  - Fixed `graphTriadCensus()` appending stale zeros on repeated runs.

### Refactoring

  - New `Graph::vertexOutNeighborsSet()`: returns enabled 1-hop out-neighbors
    in the current relation; parametric for directed/undirected use.
  - Renamed `vertexNeighborhoodList/Set` → `vertexReciprocalNeighborsList/Set`
    to reflect that only reciprocal edges are considered.

### Testing / CI

  - New `socnetv-cli` clustering kernel v6 with golden baselines and benchmark
    coverage.
  - Aligned clustering benchmarks with CLI behavior; documented `--type`
    semantics.

### Build / Packaging

  - RPM spec fixes: conditional `Qt5Compat` BuildRequires per distro family,
    correct `qt6-qttools-devel` for Fedora, dropped redundant license/doc
    macros.

## [3.4] – March 2026

### New Features
  - Two-mode sociomatrix import: correctly handles bipartite networks in parser (#15).
  - Faithful Eades (1984) Spring Embedder implementation (#207).
  - New `layoutRandomInMemory()` replaces `layoutRandom()` in force-directed pipelines (#206).

### Bug Fixes
  - Progress dialog / Cancel (#52): comprehensive fix across all computation paths:
    - Wired Cancel into centrality, prestige, reachability, walks, matrix, report, layout, clique, and subgraph computations.
    - Fixed cancel-then-retry regression (reset canceled flag + invalidate distance cache).
    - Fixed stacked progress dialogs in multi-phase computations (KK layout, matrix functions).
    - All `write*` report functions converted to `bool` return; MW slots guarded on cancel.
    - All random network generators (`Erdos-Renyi`, `Small-World`, `Scale-Free`, `Regular`, `Lattice`, `Ring-Lattice`) fixed: bool return, cancel guards, progress max corrections.
  - Layouts:
    - Fixed division-by-zero, NaN/Inf and logic errors in Kamada-Kawai layout (#198).
    - Fixed FR simmering temperature derivation from canvas width (#199).
    - Batched `setNodePos` emissions in all force-directed layouts (#205, #206).
  - Centrality:
    - Fixed eigenvector centrality isolate reset and N==0 handling (#202).
    - Fixed Information Centrality isolate handling and degenerate cases (#201).
    - Fixed wrong vertex checked for `isIsolated` in `createMatrixAdjacencyInverse()` (#190).
    - Fixed clustering coefficient computation for directed networks (#58).
    - Fixed wrong weighted flag when switching relations (#82).
  - Parsers / IO:
    - Fixed Pajek `*Matrix` header parsing for relation labels (#188).
    - Fixed Pajek multirelational export as `*Matrix` blocks (#184).
    - Fixed quoted relation name normalization in Pajek headers (#185).
    - Fixed inline GML node/edge block parsing (#186).
    - Fixed arc doubling when loading undirected DOT graphs (#187).
    - Fixed platform-dependent `weighted=true` from uninitialized `initEdgeWeight` in DOT parser (#189).
    - Fixed `Graph::setDirected()` logic bugs.
  - Fixed lattice network edge deduplication and progress tracking.
  - Fixed version comparison in update-check (component-wise instead of integer).
  - Fixed `#133` (see commit).

### Refactoring (WS4 – IO/Parser)
  - Completed WS4: IO/Parser refactor into focused modules:
    - Extracted edgelist, adjacency, UCINET DL, DOT, GML, Pajek, GraphML parsers into separate files.
    - Introduced `IGraphParseSink` explicit mutation surface and `GraphParseSinkGraph` bridge.
    - Switched GUI and headless load paths to sink-backed parser mutations.
    - Removed legacy `Parser→Graph` signal wiring.
    - `Parser::load` and adjacency parser use `ParseConfig`.

### Toolchain / Testing
  - New `socnetv-cli` schema v5 `io_roundtrip` kernel for IO/parser regression protection.
  - Added IO roundtrip timing regression to benchmarks.
  - Expanded golden comparison suite with many new IO roundtrip cases and small deterministic test networks.
  - Added `run_io_roundtrip_shipped_datasets.sh` and `run_golden_io_roundtrip.sh` scripts.
  - Added UCINET FT5 io_roundtrip golden baselines.
  - Fixed `run_golden_compares.sh` argument parsing.
  - Fixed headless parser lifetime and signal race condition.

### i18n
  - Added `update_translations.sh` script; updated translation files.

### Build / Packaging
  - Debian packaging: switched to CMake build, series-aware Qt6 deps, added OpenGL/Vulkan/XKB build deps.
  - CMake: `.qm` files now generated via `qt_add_lrelease`.
  - Fixed Windows linker `/VERSION` for PE header.
  - Help menu now links to `socnetv.org/manual/`.

  - Many bugfixes, see: [GitHub Issues](https://github.com/socnetv/app/issues?q=is%3Aissue%20state%3Aclosed%20milestone%3A3.4).
  

## [3.3] – February 2026

  - Major internal refactor: `Graph` is now a façade/coordinator; functionality has been split into focused `src/graph/*` modules.
  - Extracted and stabilized DistanceEngine; added deterministic golden regression outputs and performance benchmark guardrails.
  - New headless regression harness `socnetv-cli` (modular kernels + schema-versioned JSON):
    - distance (v1), reachability (v2), walks_matrix (v3), prominence (v4)
    - strict JSON dump/compare mode with committed baselines.
  - New feature: filter vertices by centrality and prestige indices.
  - Fixed Pajek parsing edge cases (mixed files with overlapping *Arcs/*Edges blocks).
  - Fixed UCINET/DL import edge cases (line wrapping, diagonal handling).
  - Fixed walks computation (`walksBetween()` / walks matrix parameter issues).
  - Improved tie/link counting semantics on load (canonical ties + derived SNA links; density exposed in regression JSON).
  - UI polish: improved disabled widget styling and custom SVG checkbox/radio styling.
  - Cross-platform build & packaging fixes (Qt6/CMake, Debian packaging updates, openSUSE spec fixes, macOS arm64 linker fix).
  - Many bugfixes, see: [GitHub Issues](https://github.com/socnetv/app/issues?q=is%3Aissue%20state%3Aclosed%20milestone%3A3.3).

## [3.2] – April 2025

  - Support custom attributes (metadata) in nodes (via the Node Properties dialog).
  - Support for node labels in adjacency matrix formatted files.
  - New CMake-based build system.
  - Updated look and behavior of Filter Edges by Weight functionality.
  - Many bugfixes, see: [GitHub Issues](https://github.com/socnetv/app/issues?q=is%3Aissue%20state%3Aclosed%20milestone%3A3.2).

## [3.1] – June 2023

  - Version 3.1 released, our first Qt6-only version.
  - Improved large file loading and responsiveness with large networks (>20,000 edges).
  - Reduced memory footprint.
  - Fixed edge filtering (see issue #140).
  - Enhanced "Find Node by Index Score" dialog for more meaningful comparisons.
  - Fixed numerous bugs.
  - Improved usability and help messages.


## [3.0] – July 2021

  - Version 3.0 released for Windows, macOS, and Linux, with improved graph calculation speed and new command-line parameters.
  - First version to support hardware-accelerated (OpenGL) rendering of networks.
  - Improved Web Crawler:
    - Tests for OpenSSL support in the OS and provides user hints if OpenSSL is missing.
    - Fixed delay between requests.
  - Fixed a serious bug in weighted network centrality computations (see issue #123).
  - Note: To run SocNetV 3.0 AppImage in Fedora 34 (which uses Wayland by default), use:
    `env GDK_BACKEND=x11 ./SocNetV-3.0-dev-x86_64.AppImage`
  - OBS repositories are working again. Fedora/openSUSE packages can be downloaded from:
    [OBS Repositories](https://download.opensuse.org/repositories/home:/oxy86/)


## [2.9] – June 2021

- Version 2.9 released, bugfixes
- Version 3.0 development. To run 3.0-dev in Fedora, use:
   env GDK_BACKEND=x11  ./SocNetV-3.0-dev-x86_64.AppImage


## [2.8] – Jan 2021

- Version 2.8, with some bugfixes


## [2.7] – Dec 2020

- Version 2.6 and 2.7 release with bugfixes and new features.


## [2.5] – Feb 2019

  - Version 2.5 released with new features and bugfixes.
  - Prominence scores distribution in reports and in-app mini chart.
  - Support for custom node icons (PNG, JPEG, SVG, etc).
  - Edge dichotomization algorithm.
  - High quality theme, inspired by Material Design, for uniform look and feel of SocNetV across all OSes.
  - Support for (double) edge weights in all formats.
  - Improved export to PDF and Image (with lots of new formats).
  - Improved web crawler.
  - Lattice network generator.
  - Improved memory consumption and faster measure computations.
  - Search and select multiple nodes by their numbers, labels, or prominence scores.
  - Many bug fixes.


## [2.4] – Feb 2018

- Version 2.4 released with many new features.
- New Force-Directed Placement layout: Kamada-Kawai.
- New layout type by prominence score: Node colors.
- Less clutter in visualization due to reciprocated edges. These are now being drawn in a single line.
- Improved memory consumption during user interaction with large networks
- Improved web crawler with pattern include and exclude options
- Improved Statistics Panel.
- Performance options in Settings dialog
- Improved UCINET format support (fullmatrix two-mode and edgelist).
- New "Check for updates" procedure.
- Much improved stability. See Changelog for bugs closed.


## [2.3] – Jul 2017

- Version 2.3 released with bugfixes and new features:
- Dyad and  Actor/Ego reciprocity
- Zero-weighted edge support and zero-weighted edge color selection functionality in Settings
- Bug Closed:
 - #28 Edges with values in [-1,0) are not visible
 - #29 Settings: Negative edge colour preferences break positive edge colours 


## [2.2] – Jan 2017

- Version 2.2 released with major new features.
- Hierarchical Clustering Analysis (HCA)
- Pearson correlation coefficients
- Actor Similarities
- Tie profile dissimilarities
- Maximal clique census
- New network symmetrization methods: Strong Ties, Cocitation
- Multi-relational data read and write in GraphML
- GML format support
- Support for EdgeLists with labels
- Support for Pajek multirelational directed networks
- Adjacency matrix plotting
- Better reports (in HTML with JS)
- Improved performance and GUI

## [2.1] – Sep 2016

- Version 2.1 released with a few new features but lots of bug fixes.
  This version brings a new algorithm for d-regular random network generation,
  and also a nice new dialog to control it.
  See ChangeLog for a complete log of new features and bugfixes.
- Version 2.0 released with major code overhaul, new GUI layout and lots of bugfixes and improvements.
  The new version brings stability, great performance boost, and nice new features such as separate modes
  for graphs and digraphs, permanent settings/preferences functionality, edge labeling, recent files, 
  keyboard shortcuts, etc. Also there are improvements in Force-Directed layouts, i.e. Fructherman-Reingold.
  See ChangeLog for a complete overview of the new features.
- The SocNetV Manual is now build with Doxygen and it is available at http://socnetv.sf.net/documentation

## [1.9] – June 2015

- Version 1.9 released with lots of bugfixes and a faster matrix inverse routine using LU decomposition. 
  Also Information Centrality is greatly improved in terms of computation speed.
  PageRank Prestige algorithm corrected to compute PR using the correct formula. The initial PR score 
  of each node is now 1/N.
  Bugs closed:
    #1463069 wrong average distance when there are isolates 
    #1365037 certain sparse matrices crash socnetv on invertMatrix method 
    #1365582 centralityInformation() is slow when network N>100 
    #1463095 edge filter works but the user cannot undo 
    #1464422 wrong pagerank results 
    #1464430 socnetv refuses to read pajek files not starting with *Network 
    #1465774 edges do not always follow relations 
    #1463082 edge color change is not taking place 
    #1464418 socnetv crashes on pagerank computation on isolated nodes 

- Version 1.8 released with the following new features: 
  New clique census routine to compute maximal cliques with up to 4 vertices.
  New Scale-free random generation methods. Improved Erdos-Renyi generation to include G(n,M) model. 
  Fixed bug in Clustering Coefficient - SocNetV now computes CluCof correctly in all cases.
  New improved dialogs for easy random network generation (Scale-free, Erdos-Renyi, and Small-World)
  Fixed bug in Node Properties dialog. It is now populated with current node settings.

## [1.7] – May 2015

- Version 1.7 released. New node group select/edit functionality and file previewer supporting  different codecs
- Version 1.6 released. New and improved web crawler functionality. See Changelog for more.

## [1.5] – Oct 2014

- Version 1.5 released. First version with dijkstra algorithm for the SSSP in weighted nets. See Changelog for more.

## [1.4] – Sep 2014

- Version 1.4 released. Brought new layout type (nodal size by prominence index), edgelist1 UCINET format import method and many bugfixes.

## [1.3] – Aug 2014

- Version 1.3 released.
- First time SocNetV works with multigraphs 

## [1.2] – Aug 2014

- Version 1.2 released. It features a major GUI overhaul and brings in a new "prominence indices" conceptualization based on Wasserman & Faust. 
  In general, Centrality indices focus on outLinks (choices given) while Prestige indices consider inLinks (choices received).
  Added 3 Prestige indices (Degree, Proximity and PageRank), new reachability measures (Walks, Connectedness, and Reachability Matrix) and fixed a slew of bugs in indices calculation. 
  All algorithms are now tested to report 100% correct results.
- Version 1.1 released with major bug fixes. See ChangeLog.
- First time distribution of a disk image for installation in Mac OS X

## [1.0] – Feb 2014

- Version 1.0 released, starting a new 1.x series based on Qt5. The 0.x series is no longer maintained. Please upgrade :)
- PageRank calculation and layout 
- SRS Documentation by Vagelis Motesnitsalis

## July 2013

- Moved project code to git/BB
- Started development for Qt5

## Oct 2010

- Version 0.90 released
- New Power & Information Centralities

## Jan 2010

- Version 0.80
- New List import feature
- New Triad Census feature
- Various Bug Fixes

## June 2009

- Version 0.70
- First web crawler implementation 

## May 2009

- Version 0.6 (release)
- GraphML becomes native SocNetV load format

## Feb 2009

- Version 0.51 (bugfix release)
- Version 0.50 (released)
- Small world creation
- Clustering coefficient
- Exporting to PDF
- Printing works OK.

## Jan 2009

- Version 0.49 (released)
- Ubuntu repository created. 

## Sep 2008

- New logo
- New openSUSE package repo.
- Version 0.48 released
- Version 0.47 released
- Version 0.46 released.
  Lots of bugfixes.
  New features:
  - Node sizes may reflect degree.

## Aug 2008

- New Debian Package
- Version 0.45 released. 
  New features:
  - GraphML initial support.
  - New man page and updated online documentation.
  - HtmlViewer renders online help with the help of QtWebKit (openSUSE: libQtWebKit-devel)
  - New widget for network rotation.
  - New widget for zooming replaces the old one.
  - Nodes may have 4 different shapes: circles, diamonds, triangles, boxes and ellipses are supported.
  - There was a bug in Qt 4.3 QGraphicsView causing redraw delays. Is fixed in Qt 4.4 :)
  - Cosmetic changes, i.e. new icons, new layout for the left dock.
  - Code clean-up in MainWindows Class and Matrix. 
  - Deleted obsolete members and functions such as nodeExists(), mousePosGW(), Dijkstra, etc. 
  - Bug-fixes on loading Pajek networks and layout algorithm.

## May 2008

- Version 0.44 released one year after v.0.43.
  New features: 
	Ported to Qt4: Code rewritten almost from scratch.
	Splitted MainWindow/GUI from algorithms via a new Graph Class. 
	Improved GUI with docks.
	Network zooming via mouse wheel.
	Spring Embedder: Dynamic network reallocation
	Thread support.
	Much faster calculation of distances and centralities (BFS/dijkstra).
	Betweenness centrality now is much more efficiently calculated. 
	Changed license to GPL3
	Layout in circles and levels by centrality.
	Better graphics and antialiasing (disabled - enable by pressing F8).
	New centrality index: Eccentricity.
	

## Sep 2006

- version 0.43 released with new layout features.

## June 2006

- version 0.42 released with updated help files.

## May 2006

- Did some work on the webpages at http://socnetv.org. Hope it is better now.

## March 2006

- version 0.41 released. 


## February 2006

- version 0.40 released. Efforts to be a pretty trustworthy release.
- sourceforge project downloads are more than enough daily, but there is no feedback yet for versions 0.38 and 0.39.
- version 0.39 released. Somewhat rushed release.
- constant changes in the homepage. 
- updated links in www.insna.org 


## January 2006

- version 0.38 released after one year of silence.
- The project moved to sourceforge.net
- The homepage is https://socnetv.org