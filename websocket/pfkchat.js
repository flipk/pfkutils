var PFK = {}; PFK.Chat = dcodeIO.ProtoBuf.newBuilder().import({
    "package": "PFK.Chat",
    "messages": [
        {
            "name": "Username",
            "fields": [
                {
                    "rule": "required",
                    "type": "string",
                    "name": "username",
                    "id": 1,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        },
        {
            "name": "NewUsername",
            "fields": [
                {
                    "rule": "required",
                    "type": "string",
                    "name": "oldusername",
                    "id": 1,
                    "options": {}
                },
                {
                    "rule": "required",
                    "type": "string",
                    "name": "newusername",
                    "id": 2,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        },
        {
            "name": "IM_Message",
            "fields": [
                {
                    "rule": "required",
                    "type": "string",
                    "name": "username",
                    "id": 1,
                    "options": {}
                },
                {
                    "rule": "required",
                    "type": "string",
                    "name": "msg",
                    "id": 2,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        },
        {
            "name": "ClientToServer",
            "fields": [
                {
                    "rule": "required",
                    "type": "ClientToServerType",
                    "name": "type",
                    "id": 1,
                    "options": {}
                },
                {
                    "rule": "optional",
                    "type": "Username",
                    "name": "login",
                    "id": 2,
                    "options": {}
                },
                {
                    "rule": "optional",
                    "type": "NewUsername",
                    "name": "changeUsername",
                    "id": 3,
                    "options": {}
                },
                {
                    "rule": "optional",
                    "type": "IM_Message",
                    "name": "imMessage",
                    "id": 4,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        },
        {
            "name": "UserList",
            "fields": [
                {
                    "rule": "repeated",
                    "type": "string",
                    "name": "usernames",
                    "id": 1,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        },
        {
            "name": "UserStatus",
            "fields": [
                {
                    "rule": "repeated",
                    "type": "string",
                    "name": "username",
                    "id": 1,
                    "options": {}
                },
                {
                    "rule": "required",
                    "type": "string",
                    "name": "status",
                    "id": 2,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        },
        {
            "name": "Notification",
            "fields": [
                {
                    "rule": "required",
                    "type": "string",
                    "name": "username",
                    "id": 1,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        },
        {
            "name": "ServerToClient",
            "fields": [
                {
                    "rule": "required",
                    "type": "ServerToClientType",
                    "name": "type",
                    "id": 1,
                    "options": {}
                },
                {
                    "rule": "optional",
                    "type": "UserList",
                    "name": "userList",
                    "id": 2,
                    "options": {}
                },
                {
                    "rule": "optional",
                    "type": "UserStatus",
                    "name": "userStatus",
                    "id": 3,
                    "options": {}
                },
                {
                    "rule": "optional",
                    "type": "Notification",
                    "name": "notification",
                    "id": 4,
                    "options": {}
                },
                {
                    "rule": "optional",
                    "type": "NewUsername",
                    "name": "changeUsername",
                    "id": 5,
                    "options": {}
                },
                {
                    "rule": "optional",
                    "type": "IM_Message",
                    "name": "imMessage",
                    "id": 6,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        }
    ],
    "enums": [
        {
            "name": "ClientToServerType",
            "values": [
                {
                    "name": "CTS_LOGIN",
                    "id": 1
                },
                {
                    "name": "CTS_CHANGE_USERNAME",
                    "id": 2
                },
                {
                    "name": "CTS_IM_MESSAGE",
                    "id": 3
                },
                {
                    "name": "CTS_PING",
                    "id": 4
                }
            ],
            "options": {}
        },
        {
            "name": "ServerToClientType",
            "values": [
                {
                    "name": "STC_USER_LIST",
                    "id": 1
                },
                {
                    "name": "STC_USER_STATUS",
                    "id": 2
                },
                {
                    "name": "STC_LOGIN_NOTIFICATION",
                    "id": 3
                },
                {
                    "name": "STC_LOGOUT_NOTIFICATION",
                    "id": 4
                },
                {
                    "name": "STC_CHANGE_USERNAME",
                    "id": 5
                },
                {
                    "name": "STC_IM_MESSAGE",
                    "id": 6
                },
                {
                    "name": "STC_PONG",
                    "id": 7
                }
            ],
            "options": {}
        }
    ],
    "imports": [],
    "options": {}
}).build("PFK.Chat")
