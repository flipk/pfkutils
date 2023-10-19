var PFK = {}; PFK.TestMsgs = dcodeIO.ProtoBuf.newBuilder().import({
    "package": "PFK.TestMsgs",
    "messages": [
        {
            "name": "CommandAdd_m",
            "fields": [
                {
                    "rule": "required",
                    "type": "int32",
                    "name": "a",
                    "id": 1,
                    "options": {}
                },
                {
                    "rule": "required",
                    "type": "int32",
                    "name": "b",
                    "id": 2,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        },
        {
            "name": "Command_m",
            "fields": [
                {
                    "rule": "required",
                    "type": "CommandType",
                    "name": "type",
                    "id": 1,
                    "options": {}
                },
                {
                    "rule": "optional",
                    "type": "CommandAdd_m",
                    "name": "add",
                    "id": 2,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        },
        {
            "name": "ResponseAdd_m",
            "fields": [
                {
                    "rule": "required",
                    "type": "int32",
                    "name": "sum",
                    "id": 1,
                    "options": {}
                }
            ],
            "enums": [],
            "messages": [],
            "options": {}
        },
        {
            "name": "Response_m",
            "fields": [
                {
                    "rule": "required",
                    "type": "ResponseType",
                    "name": "type",
                    "id": 1,
                    "options": {}
                },
                {
                    "rule": "optional",
                    "type": "ResponseAdd_m",
                    "name": "add",
                    "id": 2,
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
            "name": "CommandType",
            "values": [
                {
                    "name": "COMMAND_ADD",
                    "id": 1
                }
            ],
            "options": {}
        },
        {
            "name": "ResponseType",
            "values": [
                {
                    "name": "RESPONSE_ADD",
                    "id": 1
                }
            ],
            "options": {}
        }
    ],
    "imports": [],
    "options": {},
    "services": []
}).build("PFK.TestMsgs");
