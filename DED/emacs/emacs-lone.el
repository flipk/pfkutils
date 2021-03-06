;
; emacs.el
;

(custom-set-variables
  ;; custom-set-variables was added by Custom -- don't edit or cut/paste it!
  ;; Your init file should contain only one such instance.
 '(blink-matching-paren-distance nil)
 '(display-time-format "%H:%M")
 '(display-time-mail-file (quote false))
 '(file-precious-flag t)
 '(menu-bar-mode t)
 '(tool-bar-mode nil nil (tool-bar))
 '(mode-line-format (quote (" " mode-line-mule-info mode-line-modified " " mode-line-buffer-identification " " global-mode-string " %[(" mode-name mode-line-process minor-mode-alist "%n" ")%] " (line-number-mode "L%l ") (column-number-mode "C%c ") (-3 . "%p"))))
 '(mode-line-inverse-video t)
)

(line-number-mode 1)
(column-number-mode 1)
(display-time)

(cond (window-system
       (progn
         (setq
	  special-display-buffer-names
	  '("*compilation*" "*shell*")
	  )
         (setq
	  hilit-mode-enable-list  '(not text-mode)
	  hilit-background-mode   'dark
	  hilit-inhibit-hooks     nil
	  hilit-inhibit-rebinding nil
	  hilit-quietly           t)
         (require 'hilit19)))
      ((not window-system)
       (progn
         (menu-bar-mode -1))))

(defvar c-mode-indent 4)
(defvar c-mode-tabs 0)
(defvar c-mode-tabwidth 4)

(defun set-c-mode-thingies () "setup for c mode"
  (interactive)
  (c-set-style "BSD")
  (let ((tabs-mode (if (= c-mode-tabs 0) nil 't)))
    (setq c-basic-offset c-mode-indent
	  indent-tabs-mode tabs-mode
	  tab-width c-mode-tabwidth)
    (local-set-key "\C-u" 'fix-buffer-my-way)))

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

(defun fix-buffer-my-way ()
  ""
  (interactive)
  (progn
    (if (= c-mode-tabs 1)
	(tabify (point-min) (point-max))
        (untabify (point-min) (point-max)))
    (indent-region (point-min) (point-max) nil)))

(defun showhelp () "Show help string"
  (interactive)
  (message " f1=help f2=fill-region f4=setcmode f6=eval f7=list-buffers f8=execute f9=make-frame f10=delete-frame"))

(global-set-key [f1]  'showhelp)
(global-set-key [f2]  'fill-region)
(global-set-key [f4]  'set-c-mode)
(global-set-key [f6]  'eval-last-sexp)
(global-set-key [f7]  'list-buffers)
(global-set-key [f8]  'execute-extended-command)
(global-set-key [f9]  'make-frame)
(global-set-key [f10] 'delete-frame)
(global-set-key "\C-g" 'goto-line)
(global-set-key "\C-x\C-w" 'make-dumb-frame)
(global-set-key "\C-x\C-y" 'clearcase-checkout-file)
(global-set-key "\C-xy" 'cscope-window)
(global-set-key "\C-xY" 'cscope-rebuild)

; (call-process "argv[0]" infile buffer display "argv[1]" "argv[2]"...)
; infile describes the file to use as stdin
; buffer means buffer to place output into
;  - t means current buffer
;  - nil means discard
;  - 0 means discard and don't wait
; display should be nil (don't update output window)

(defun cscope-window ()
  "open cscope in an xterm"
  (interactive)
  (call-process "xterm" nil 0 nil
		"-g" "140x45+225+20"
		"-e" "/home/pknaack1/bin/myemacs-cscope-helper"))

(defun cscope-rebuild ()
  "rebuild cscope database"
  (interactive)
  (call-process "xterm" nil 0 nil
		"-g" "80x10+200+200"
		"-e" "/home/pknaack1/bin/myemacs-cscope-rebuild-helper"))

(defun clearcase-checkout-file ()
  "checkout a clearcase file"
  (interactive)
  (let ((fname (buffer-file-name)))
    (if buffer-read-only
	(progn
	  (toggle-read-only)
	  (message "About to checkout file %s..." fname)
	  (call-process "xterm" nil 0 nil
			"-g" "80x10+200+200" "-e"
			(concat
			 "/home/pknaack1/bin/myemacs-checkout-helper "
			 (buffer-file-name)))
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

(put 'downcase-region 'disabled nil)
(put 'upcase-region 'disabled nil)

(garbage-collect)
(message "Press F1 for a list of local keybindings.")
