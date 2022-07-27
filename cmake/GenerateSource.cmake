file(READ ${FROM_FILE} EMPLACE_SOURCE_HERE)
configure_file("${OVER_FILE}" "${TO_FILE}" @ONLY)
