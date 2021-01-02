(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(blink-matching-paren-distance nil)
 '(column-number-mode t)
 '(compilation-scroll-output t)
 '(compile-command (concat (getenv "HOME") "/pfk/bin/myemacs-compile"))
 '(cscope-close-window-after-select nil)
 '(cscope-option-kernel-mode t)
 '(display-time-format "%H:%M")
 '(display-time-mail-file (quote false))
 '(display-time-mode t)
 '(inhibit-startup-screen t)
 '(menu-bar-mode nil)
 '(inhibit-startup-echo-area-message (getenv "USER"))
 '(initial-scratch-message "")
 '(ivy-use-virtual-buffers t)
 '(mode-line-format
   (quote
    (" " mode-line-mule-info mode-line-modified " " mode-line-buffer-identification " " global-mode-string " %[(" mode-name mode-line-process minor-mode-alist "%n" ")%] "
     (line-number-mode "L%l ")
     (column-number-mode "C%c ")
     (-3 . "%p"))))
 '(mode-line-inverse-video t)
 '(mouse-buffer-menu-maxlen 100)
 '(mouse-buffer-menu-mode-mult 100)
 '(speedbar-show-unknown-files t)
 '(verilog-auto-delete-trailing-whitespace t)
 '(verilog-auto-inst-column 10)
 '(verilog-auto-newline nil)
 '(verilog-indent-begin-after-if nil)
 '(package-selected-packages
   (quote
    (counsel git-gutter git-timemachine ivy protobuf-mode swiper xcscope magit-lfs magit)))
 '(tool-bar-mode nil))
(custom-set-faces
 ;; custom-set-faces was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 )
