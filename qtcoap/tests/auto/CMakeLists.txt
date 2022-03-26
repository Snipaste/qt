# Generated from auto.pro.

# special case begin
# add_subdirectory(cmake)
# sepcial case end
add_subdirectory(qcoapclient)
add_subdirectory(qcoapmessage)
add_subdirectory(qcoapoption)
add_subdirectory(qcoaprequest)
add_subdirectory(qcoapresource)
if(QT_FEATURE_private_tests)
    add_subdirectory(qcoapqudpconnection)
    add_subdirectory(qcoapinternalrequest)
    add_subdirectory(qcoapinternalreply)
    add_subdirectory(qcoapreply)
endif()
