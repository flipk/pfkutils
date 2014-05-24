
DEST=/home/nginx/html-chat-sec
BINDEST=/home/nginx/sbin/

all:
	cd ../libWebAppServer && make
	cd server && make

install:
	cd html && cp favicon.ico pfk.png $(DEST)
	cd html/app && cp *.js *.gif *.css *.html $(DEST)/chat/
	cd html/login && cp *.js *.css *.html $(DEST)/chat_login/
	cd server && cp server_pfkchat $(BINDEST)
	cd server && cp pfkchat-messages-full.js $(DEST)/chat/messages.js
	cd server && cp pfkchat-messages-login.js $(DEST)/chat_login/messages.js
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

clean:
	cd ../libWebAppServer && make clean
	cd server && make clean
