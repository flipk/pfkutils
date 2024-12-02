
(package-initialize)
(setq custom-file (concat (getenv "HOME") "/pfk/etc/emacs/emacs-custom.el"))
(load custom-file)

;for FUTURE REFERENCE
;(cond ((< emacs-major-version 22)
;       ;; Emacs 21 customization.
;       (setq custom-file "~/.custom-21.el"))
;      ((and (= emacs-major-version 22)
;	    (< emacs-minor-version 3))
;       ;; Emacs 22 customization, before version 22.3.
;       (setq custom-file "~/.custom-22.el"))
;      (t
;       ;; Emacs version 22.3 or later.
;       (setq custom-file "~/.emacs-custom.el")))

(require 'package)
(add-to-list 'package-archives '("melpa" . "http://melpa.org/packages/"))
(add-to-list 'package-archives '("org" . "http://orgmode.org/elpa/") t)

(cond (window-system
       (progn
         (let ((desktop (getenv "EMACS_NUMBER")))
	   (setq frame-title-format
		 (list "emacs" desktop " %b "
		       user-login-name "@" system-name))
           (let ((minibuftitle (concat "Emacs Minibuffer" desktop)))
             (setq
              default-frame-alist
              '(
; this stuff commented out here tears the minibuffer off the
; bottom of the window and makes it a separate singleton window
; for the whole emacs instance (on this display).
;               (minibuffer . nil) (width . 81)
                (cursor-color . "red"))
;             initial-frame-alist 'nil
;             minibuffer-frame-alist
;             (cons (cons 'title minibuftitle)
;                   '((top . 0) (left . 20) (width . 155) (height . 1)
;                     (auto-raise . t) (minibuffer-lines . 1)
;                     (vertical-scroll-bars . nil)
;                     (name . "Emacs Minibuffer")))

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

(setq mouse-buffer-menu-mode-groups
      '((".*" . "Buffers")))

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
;                            face dired-directory
)))

(add-hook 'shell-mode-hook 'add-mode-line-dirtrack)

(add-to-list 'auto-mode-alist '("\\.scss\\'" . css-mode))
(add-to-list 'auto-mode-alist '("\\.h\\'" . c++-mode))
(add-to-list 'auto-mode-alist '("\\.proto\\'" . protobuf-mode))

(defun set-verilog-mode-thingies () "setup for verilog mode"
  (interactive)
  (setq indent-tabs-mode nil)
  (local-set-key "\C-u" 'fix-verilog-buffer-my-way))

(add-hook 'verilog-mode-hook 'set-verilog-mode-thingies)

(add-hook 'sh-set-shell-hook 'set-sh-mode-thingies)
(defun set-sh-mode-thingies () "setup for sh mode"
  (setq indent-tabs-mode nil)
  (message "setting sh mode thingies"))

(setq pfk-regular-font "pfk13")
(setq pfk-regular-bold-font "pfk13") ; pfk13bold !
(setq pfk-big-font "pfk20")
(setq pfk-big-bold-font "pfk20bold")

(setq pfk-make-big-flag nil)

(defun my-make-big () "make window font bigger"
  (interactive)
  (if pfk-make-big-flag
      (progn
        (message (concat "setting frame font to " pfk-regular-font))
        (set-frame-font pfk-regular-font)
        (setq pfk-make-big-flag nil))
    (progn
        (message (concat "setting frame font to " pfk-big-font))
     (set-frame-font pfk-big-font)
     (setq pfk-make-big-flag t))))

(global-set-key [f3]  'my-make-big)

(global-set-key [f2]  'fill-region)
(global-set-key [f6]  'eval-last-sexp)
(global-set-key [f9]  'make-frame)
(global-set-key [f10] 'delete-frame)
(global-set-key "\C-g" 'goto-line)

(put 'downcase-region 'disabled nil)
(put 'upcase-region 'disabled nil)
(put 'scroll-left 'disabled nil)
(put 'erase-buffer 'disabled nil)

; useful without ivy : (ido-mode)
(ivy-mode 1)
; broken? (global-git-gutter-mode +1)
; broken? (cscope-setup)

(defun isearch-or-swiper ()
  "calls either isearch-foward or swiper based on size of buffer"
  (interactive)
  (if (> (buffer-size) 500000)
     (isearch-forward)
   (swiper)))

(global-set-key (kbd "C-x C-b") 'ibuffer)
(global-set-key (kbd "C-s") 'isearch-or-swiper)
(global-set-key (kbd "C-x C-f") 'counsel-find-file)
(global-set-key (kbd "M-x") 'counsel-M-x)
(global-set-key (kbd "M-y") 'counsel-yank-pop)
(global-set-key [S-f9] 'compile)
(global-set-key [f8] 'next-error)
(global-set-key [S-f8] 'previous-error)
(global-set-key (kbd "C-x g") 'magit-status)

(garbage-collect)
