
DEST=/home/nginx/html-chat-sec
DEST2=/home/nginx/html-blade
BINDEST=/home/nginx/sbin/

all:
	cd ../libWebAppServer/build && make
	cd server && make
	cd test && make

install:
	cd html && cp favicon.ico pfk.png $(DEST)
	cd html/app && cp *.js *.gif *.css *.html $(DEST)/chat/
	cd html/login && cp *.js *.css *.html $(DEST)/chat_login/
	cd html/test && cp *.js *.html $(DEST2)/
	cd server && cp server_pfkchat $(BINDEST)
	cd html && \
		tar cf - app | tar -C $(DEST) -xf - && \
		rm -rf $(DEST)/chat && \
		mv $(DEST)/app $(DEST)/chat
	cd html && \
		tar cf - login | tar -C $(DEST) -xf - && \
		rm -rf $(DEST)/chat_login && \
		mv $(DEST)/login $(DEST)/chat_login
	cd html && \
		rm -rf $(DEST)/img && \
		tar cf - img | tar -C $(DEST) -xf -
	cd server && cp pfkchat-messages-full.js $(DEST)/chat/messages.js
	cd server && cp pfkchat-messages-login.js $(DEST)/chat_login/messages.js

clean:
	cd ../libWebAppServer && make clean
	cd server && make clean
