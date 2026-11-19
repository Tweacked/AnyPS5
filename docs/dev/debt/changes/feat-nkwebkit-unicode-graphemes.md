# libSceNKWebKit

## add Unknown function info

- [UTF-16 grapheme counting](../../../core/libs/prx/libSceNKWebKit/src/GraphemeClusters.cpp) (libSceNKWebKit) - Uses the default extended grapheme rules in [Unicode 17 UAX #29](https://www.unicode.org/reports/tr29/tr29-47.html#Grapheme_Cluster_Boundary_Rules) and the bundled ICU's Unicode properties. [PS5 WebKit](https://github.com/neonmodder123/PS5-WebKit-11.x/blob/80f77e94c63ee7202eec3d4b5b1a021dd89011ba/Source/WTF/wtf/text/TextBreakIterator.cpp) uses a character break iterator. Console ICU data and any differences in boundaries remain unmeasured. Closes when: grapheme counts and UTF-16 offsets are compared with a console module.
