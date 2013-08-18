"use strict";
/** @suppress {duplicate}*/var PFK;
if (typeof(PFK)=="undefined") {PFK = {};}
if (typeof(PFK.Chat)=="undefined") {PFK.Chat = {};}

PFK.Chat.Username = PROTO.Message("PFK.Chat.Username",{
	username: {
		options: {},
		multiplicity: PROTO.required,
		type: function(){return PROTO.string;},
		id: 1
	}});
PFK.Chat.NewUsername = PROTO.Message("PFK.Chat.NewUsername",{
	oldusername: {
		options: {},
		multiplicity: PROTO.required,
		type: function(){return PROTO.string;},
		id: 1
	},
	newusername: {
		options: {},
		multiplicity: PROTO.required,
		type: function(){return PROTO.string;},
		id: 2
	}});
PFK.Chat.IM_Message = PROTO.Message("PFK.Chat.IM_Message",{
	username: {
		options: {},
		multiplicity: PROTO.required,
		type: function(){return PROTO.string;},
		id: 1
	},
	msg: {
		options: {},
		multiplicity: PROTO.required,
		type: function(){return PROTO.string;},
		id: 2
	}});
PFK.Chat.ClientToServer = PROTO.Message("PFK.Chat.ClientToServer",{
	ClientToServerType: PROTO.Enum("PFK.Chat.ClientToServer.ClientToServerType",{
		LOGIN :1,
		CHANGE_USERNAME :2,
		IM_MESSAGE :3,
		PING :4	}),
	type: {
		options: {},
		multiplicity: PROTO.required,
		type: function(){return PFK.Chat.ClientToServer.ClientToServerType;},
		id: 1
	},
	login: {
		options: {},
		multiplicity: PROTO.optional,
		type: function(){return PFK.Chat.Username;},
		id: 2
	},
	changeUsername: {
		options: {},
		multiplicity: PROTO.optional,
		type: function(){return PFK.Chat.NewUsername;},
		id: 3
	},
	imMessage: {
		options: {},
		multiplicity: PROTO.optional,
		type: function(){return PFK.Chat.IM_Message;},
		id: 4
	}});
PFK.Chat.UserList = PROTO.Message("PFK.Chat.UserList",{
	username: {
		options: {},
		multiplicity: PROTO.repeated,
		type: function(){return PROTO.string;},
		id: 1
	}});
PFK.Chat.UserStatus = PROTO.Message("PFK.Chat.UserStatus",{
	username: {
		options: {},
		multiplicity: PROTO.repeated,
		type: function(){return PROTO.string;},
		id: 1
	},
	status: {
		options: {},
		multiplicity: PROTO.required,
		type: function(){return PROTO.string;},
		id: 2
	}});
PFK.Chat.Notification = PROTO.Message("PFK.Chat.Notification",{
	username: {
		options: {},
		multiplicity: PROTO.required,
		type: function(){return PROTO.string;},
		id: 1
	}});
PFK.Chat.ServerToClient = PROTO.Message("PFK.Chat.ServerToClient",{
	ServerToClientType: PROTO.Enum("PFK.Chat.ServerToClient.ServerToClientType",{
		USER_LIST :1,
		USER_STATUS :2,
		LOGIN_NOTIFICATION :3,
		LOGOUT_NOTIFICATION :4,
		CHANGE_USERNAME :5,
		IM_MESSAGE :6	}),
	type: {
		options: {},
		multiplicity: PROTO.required,
		type: function(){return PFK.Chat.ServerToClient.ServerToClientType;},
		id: 1
	},
	userList: {
		options: {},
		multiplicity: PROTO.optional,
		type: function(){return PFK.Chat.UserList;},
		id: 2
	},
	userStatus: {
		options: {},
		multiplicity: PROTO.optional,
		type: function(){return PFK.Chat.UserStatus;},
		id: 3
	},
	notification: {
		options: {},
		multiplicity: PROTO.optional,
		type: function(){return PFK.Chat.Notification;},
		id: 4
	},
	changeUsername: {
		options: {},
		multiplicity: PROTO.optional,
		type: function(){return PFK.Chat.NewUsername;},
		id: 5
	},
	imMessage: {
		options: {},
		multiplicity: PROTO.optional,
		type: function(){return PFK.Chat.IM_Message;},
		id: 6
	}});
