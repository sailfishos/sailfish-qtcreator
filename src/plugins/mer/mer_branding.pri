isEmpty(BASE_OS_NAME) {
    DEFINES += MER_BASE_OS_NAME=\\\"Sailfish\\\"
} else {
    DEFINES += MER_BASE_OS_NAME=\\\"$$BASE_OS_NAME\\\"
}
