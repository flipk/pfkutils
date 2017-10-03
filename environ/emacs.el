;
; emacs.el
;

(custom-set-variables
 '(display-time-format "%H:%M")
 '(mode-line-format (quote (" " mode-line-mule-info mode-line-modified " " mode-line-buffer-identification " " global-mode-string " %[(" mode-name mode-line-process minor-mode-alist "%n" ")%] " (line-number-mode "L%l ") (column-number-mode "C%c ") (-3 . "%p"))))
 '(file-precious-flag t)
 '(display-time-mail-file (quote false))
 '(mode-line-inverse-video t)
 '(blink-matching-paren-distance nil))

(custom-set-faces)

(load "~/.elisp/myshell.el")
(load "~/.elisp/myfiles.el")

(line-number-mode 1)
(column-number-mode 1)

(cond (window-system
       (progn
         (setq
	  default-frame-alist '((minibuffer . nil))
	  initial-frame-alist 'nil
	  minibuffer-frame-alist
	  '((top . -1) (left . -1) (width . 155) (height . 1)
	    (auto-raise . t) (minibuffer-lines . 1)
	    (vertical-scroll-bars . nil)
	    (name . "Emacs Minibuffer")
	    (title . "Emacs Minibuffer"))
	  special-display-buffer-names
;	  '("*compilation*" "*shell*" "*Buffer List*")
	  '("*compilation*" "*shell*")
	  )
         (setq
	  hilit-mode-enable-list  '(not text-mode)
	  hilit-background-mode   'dark
	  hilit-inhibit-hooks     nil
	  hilit-inhibit-rebinding nil
	  hilit-quietly           t)
         (require 'hilit19)
         (load "~/.elisp/myserver.el")
         (server-start)
         (display-time)))
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
(global-set-key [f4]  'set-c-mode)

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
(global-set-key [f6]  'eval-last-sexp)
(global-set-key [f7]  'list-buffers)
(global-set-key [f8]  'execute-extended-command)
(global-set-key [f9]  'make-frame)
(global-set-key [f10] 'delete-frame)
(global-set-key "\C-g" 'goto-line)
(global-set-key "\C-x\C-w" 'make-dumb-frame)
(global-set-key "\C-x\C-m" 'compile)

(defun make-dumb-frame ()
  "Make a frame with no menu bar and no scroll bar."
  (interactive)
  (make-frame '((menu-bar-lines . 0)
		(vertical-scroll-bars . nil)
		(width . 50) (height . 10))))

(defun ask-user-about-supersession-threat (fn)
  "overwrite emacs default version of this function"
  't)

;; (global-set-key [f19] 'make-frame)
;;(global-set-key [f20] 'delete-frame)
(global-set-key [f35] 'scroll-up)
(global-set-key [f29] 'scroll-down)
(global-set-key [f27] 'beginning-of-buffer)
(global-set-key [f33] 'end-of-buffer)

(garbage-collect)
(message "Press F1 for a list of local keybindings.")
