;
; emacs.el
;

(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(blink-matching-paren-distance nil)
 '(column-number-mode t)
 '(display-time-format "%H:%M")
 '(display-time-mail-file (quote false))
 '(display-time-mode t)
 '(file-precious-flag t)
 '(inhibit-startup-buffer-menu t)
 '(inhibit-startup-echo-area-message (getenv "USER"))
 '(inhibit-startup-screen t)
 '(initial-scratch-message "")
 '(menu-bar-mode nil)
 '(mode-line-format (quote (" " mode-line-mule-info mode-line-modified " " mode-line-buffer-identification " " global-mode-string " %[(" mode-name mode-line-process minor-mode-alist "%n" ")%] " (line-number-mode "L%l ") (column-number-mode "C%c ") (-3 . "%p"))))
 '(mode-line-inverse-video t)
 '(mouse-buffer-menu-maxlen 100)
 '(mouse-buffer-menu-mode-mult 100)
 '(speedbar-show-unknown-files t)
 '(tool-bar-mode nil nil (tool-bar))
 '(verilog-auto-delete-trailing-whitespace t)
 '(verilog-auto-inst-column 10)
 '(verilog-auto-newline nil)
 '(verilog-indent-begin-after-if nil))

(load-file (concat "/home/" (getenv "USER")
		   "/pfk/etc/go-mode.el"))

(setq mouse-buffer-menu-mode-groups
      '((".*" . "Buffers")))

(custom-set-faces
 ;; custom-set-faces was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(cursor ((t (:background "white" :foreground "red"))))
 '(menu ((((type x-toolkit)) (:background "grey85" :foreground "black"))))
 '(mouse ((t (:background "white" :foreground "black"))))
 '(scroll-bar ((t (:background "grey85" :foreground "red")))))

(defun my-init-hook ()
  ""
  (progn
;    (message "my-init-hook was run")
    ))

(add-hook 'after-init-hook 'my-init-hook)

(setq pfk-small-font
      "-misc-*-*-r-semicondensed-*-13-120-*-*-*-*-iso8859-1")
(setq pfk-minibuf-font
      "-misc-*-*-r-semicondensed-*-13-120-*-*-*-*-iso8859-1")
(setq pfk-big-font
      "-misc-*-medium-r-normal-*-20-200-*-*-*-*-iso8859-1")
;(setq pfk-small-font "Source Code Pro Medium-9")
;(setq pfk-minibuf-font "Source Code Pro Medium-10")
;(setq pfk-big-font   "Source Code Pro Medium-14")

(defun my-fix-minibuffer ()
  ""
  (progn
    (select-frame default-minibuffer-frame)
    (set-frame-font pfk-minibuf-font)
    ))

(run-at-time 1 nil 'my-fix-minibuffer)

(defun my-make-frame-hook (frame)
  ""
  (interactive)
  (progn
    (select-frame frame)
    (set-frame-font pfk-small-font)
    (set-foreground-color "yellow")
    (set-background-color "black")
    (set-cursor-color "red")
    ))

(add-hook 'after-make-frame-functions 'my-make-frame-hook)

(cond (window-system
       (progn
	 (let ((desktop (getenv "EMACS_NUMBER")))
	   (let ((minibuftitle (concat "Emacs Minibuffer" desktop)))
	     (setq
	      default-frame-alist '((minibuffer . nil)
				    (width . 81)
				    (cursor-color . "red"))
	      initial-frame-alist 'nil
	      minibuffer-frame-alist
	      (cons (cons 'title minibuftitle)
		    '((top . 0) (left . 20) (width . 155) (height . 1)
		      (auto-raise . t) (minibuffer-lines . 1)
		      (vertical-scroll-bars . nil)
		      (name . "Emacs Minibuffer")))
;	      special-display-buffer-names '("*compilation*" "*shell*")
	      hilit-mode-enable-list  '(not text-mode)
	      hilit-background-mode   'dark
	      hilit-inhibit-hooks     nil
	      hilit-inhibit-rebinding nil
	      hilit-quietly           t)
	     (if desktop
		 (setq server-socket-dir
		       (concat "/tmp/"
			       (getenv "USER")
			       "/emacs-"
			       desktop)))
	     (server-start)))))
      ((not window-system)
       (progn
         (menu-bar-mode -1))))

(defvar c-mode-indent 4)
(defvar c-mode-tabs 0)
(defvar c-mode-tabwidth 8)

(defun set-c-mode-thingies () "setup for c mode"
  (interactive)
  (c-set-style "BSD")
  (let ((tabs-mode (if (= c-mode-tabs 0) nil 't)))
    (setq c-basic-offset c-mode-indent
	  indent-tabs-mode tabs-mode
	  tab-width c-mode-tabwidth)
    (local-set-key "\C-u" 'fix-c-buffer-my-way)))

(defun set-c-mode ( my-indent my-tabs my-tabwidth )
  "Set the C mode parameters"
  (interactive
"nEnter c-mode-indent value (4 or 8):
nEnter c-mode-tabs value (0 or 1):
nEnter c-mode-tabwidth value (4 or 8):")
  (setq c-progress-interval 1
	c-mode-indent my-indent
	c-mode-tabs my-tabs
	c-mode-tabwidth my-tabwidth)
  (set-c-mode-thingies))

(add-hook 'c-mode-hook   'set-c-mode-thingies)
(add-hook 'c++-mode-hook 'set-c-mode-thingies)

; let emacs track CWD changes by examining the shell prompt

(setq pfkdir "")

(defun my-track-shell-dir (text)
  (if (string-match "SHELL_CWD_BEGIN:\\(.*\\):SHELL_CWD_END\n" text)
      (let ((dir-start-pos (match-beginning 1))
	    (dir-end-pos (match-end 1))
	    (marker-start-pos (match-beginning 0))
	    (marker-end-pos (match-end 0))
	    (len (length text))
	    (ret "")
	    (new-dir ""))
	(setq new-dir (substring text dir-start-pos dir-end-pos))
	(cd new-dir)
	(message (concat "shell directory is " new-dir))
	(if (not (equal marker-start-pos 0))
	    (setq ret (substring text 0 marker-start-pos)))
	(if (not (equal marker-end-pos len))
	    (setq ret (concat ret (substring text marker-end-pos len))))
	ret)
    text))

(defun my-shell-setup ()
  "Track current directory"
  (add-hook 'comint-preoutput-filter-functions 'my-track-shell-dir nil t))

(setq shell-mode-hook 'my-shell-setup)

; show shell CWD in modeline

(defun add-mode-line-dirtrack ()
  (add-to-list 'mode-line-buffer-identification 
	       '(:propertize (" " default-directory " ")
;			     face dired-directory
)))

(add-hook 'shell-mode-hook 'add-mode-line-dirtrack)

(add-to-list 'auto-mode-alist ("\\.scss\\'" . css-mode))

;

(defun fix-verilog-buffer-my-way ()
  ""
  (interactive)
  (progn
    (indent-region (point-min) (point-max) nil)
    (untabify (point-min) (point-max))))

(defun set-verilog-mode-thingies () "setup for verilog mode"
  (interactive)
  (setq indent-tabs-mode nil)
  (local-set-key "\C-u" 'fix-verilog-buffer-my-way))

(add-hook 'verilog-mode-hook 'set-verilog-mode-thingies)

(defun fix-c-buffer-my-way ()
  ""
  (interactive)
  (progn
    (if (= c-mode-tabs 1)
	(tabify (point-min) (point-max))
        (untabify (point-min) (point-max)))
    (indent-region (point-min) (point-max) nil)))

(setq pfk-make-big-flag nil)

(defun my-make-big () "make window font bigger"
  (interactive)
  (if pfk-make-big-flag
      (progn
	(message (concat "setting frame font to " pfk-small-font))
	(set-frame-font pfk-small-font)
	(setq pfk-make-big-flag nil))
    (progn
	(message (concat "setting frame font to " pfk-big-font))
     (set-frame-font pfk-big-font)
     (setq pfk-make-big-flag t))))

(defun showhelp () "Show help string"
  (interactive)
  (message " f1=help f2=fill-region f3=make-big f4=setcmode f5=speedbar f6=eval f9=make-frame f10=delete-frame Cg=goto-line CxCw=dumb CxCy=co Cxy=cscope CxY=rebuild"))

(global-set-key [f1]  'showhelp)
(global-set-key [f2]  'fill-region)
(global-set-key [f3]  'my-make-big)
(global-set-key [f4]  'set-c-mode)
(global-set-key [f5]  'speedbar)
(global-set-key [f6]  'eval-last-sexp)
; f7 useless in my window manager
; f8 useless in a vnc session
(global-set-key [f9]  'make-frame)
(global-set-key [f10] 'delete-frame)
(global-set-key "\C-g" 'goto-line)
(global-set-key "\C-x\C-w" 'make-dumb-frame)
(global-set-key "\C-x\C-y" 'clearcase-checkout-file)
(global-set-key "\C-xy" 'cscope-window)
(global-set-key "\C-xY" 'cscope-rebuild)

(global-set-key [C-tab] 'buffer-menu)

; (call-process "argv[0]" infile buffer display "argv[1]" "argv[2]"...)
; infile describes the file to use as stdin
; buffer means buffer to place output into
;  - t means current buffer
;  - nil means discard
;  - 0 means discard and don't wait
; display should be nil (don't update output window)

(defun cscope-window ()
  "open cscope in a pfkterm"
  (interactive)
  (call-process "pfkterm" nil 0 nil
		"-g" "185x70+20+20"
		"-e" 
		(concat "/home/" (getenv "USER")
			"/pfk/bin/myemacs-cscope-helper")))

(defun cscope-rebuild ()
  "rebuild cscope database"
  (interactive)
  (call-process "pfkterm" nil 0 nil
		"-g" "80x10+200+200"
		"-e" 
		(concat "/home/" (getenv "USER")
			"/pfk/bin/myemacs-cscope-rebuild-helper")))

(defun clearcase-checkout-file ()
  "checkout a clearcase file"
  (interactive)
  (let ((fname (buffer-file-name)))
    (if buffer-read-only
	(progn
	  (toggle-read-only)
	  (message "About to checkout file %s..." fname)
	  (call-process "pfkterm" nil 0 nil
			"-g" "80x10+200+200" "-e"
			(concat "/home/"
				(getenv "USER")
				"/pfk/bin/myemacs-checkout-helper")
			(buffer-file-name))
	  (message "Checked out file %s." fname))
      (message "File is already checked out?"))))

(defun make-dumb-frame ()
  "Make a frame with no menu bar and no scroll bar."
  (interactive)
  (make-frame '((menu-bar-lines . 0)
		(vertical-scroll-bars . nil)
		(width . 50) (height . 10))))

(defun ask-user-about-supersession-threat (fn)
  "overwrite emacs default version of this function"
  't)

(global-set-key [f35] 'scroll-up)
(global-set-key [f29] 'scroll-down)
(global-set-key [f27] 'beginning-of-buffer)
(global-set-key [f33] 'end-of-buffer)

(setq frame-title-format (list "emacs" (getenv "EMACS_NUMBER") "%*%b %m"))

(put 'downcase-region 'disabled nil)
(put 'upcase-region 'disabled nil)
(put 'scroll-left 'disabled nil)
(put 'erase-buffer 'disabled nil)

(garbage-collect)
(message "Press F1 for a list of local keybindings.")

