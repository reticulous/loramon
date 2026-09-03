include(${CMAKE_CURRENT_LIST_DIR}/spangap_requires.cmake)
# The firmware half is entirely conditional: esp-idf/conditional/<name>/ is
# compiled only when <name> is staged, and the only slice here is the LCD app
# under conditional/spangap-lcd/. A headless build therefore registers this
# component with no sources at all — the browser half is the whole viewer —
# and the when:-gated LoraMonApp service entry keeps the trampoline out too.
idf_component_register(
    SRCS
        ${SPANGAP_CONDITIONAL_SRCS}
    INCLUDE_DIRS
        ${SPANGAP_CONDITIONAL_INCLUDE_DIRS}
    REQUIRES
        ${SPANGAP_REQUIRES}
)
