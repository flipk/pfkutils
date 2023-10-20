(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(blink-matching-paren-distance nil)
 '(column-number-mode t)
 '(compilation-scroll-output t)
 '(cscope-close-window-after-select nil)
 '(cscope-option-kernel-mode t)
 '(display-time-format "%H:%M")
 '(display-time-mail-file (quote false))
 '(display-time-mode t)
 '(face-font-family-alternatives
   (quote
    (("pfk" "courier" "fixed" "Monospace")
     ("pfk" "CMU Typewriter Text" "fixed" "courier")
     ("pfk" "helv" "helvetica" "arial" "fixed" "Sans Serif")
     ("pfk" "helvetica" "arial" "fixed" "helv"))))
 '(face-font-registry-alternatives
   (quote
    (("pfk" "gb2312.1980" "gb2312.80&gb8565.88" "gbk" "gb18030")
     ("pfk" "jisx0208.1990" "jisx0208.1983" "jisx0208.1978")
     ("pfk" "ksc5601.1989" "ksx1001.1992" "ksc5601.1987")
     ("pfk" "muletibetan-2" "muletibetan-0"))))
 '(ibuffer-formats
   (quote
    ((mark modified read-only " "
           (name 47 47 :left :elide)
           " "
           (size 9 -1 :right)
           " "
           (mode 16 16 :left :elide))
     (mark " "
           (name 16 -1)
           " " filename))))
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
   '(code-library counsel-projectile dap-mode flycheck flycheck-clang-analyzer flycheck-clangcheck flycheck-inline git-gutter git-timemachine ibuffer-git ibuffer-project ibuffer-projectile ivy-todo lsp-mode magit magit-lfs org-doing org-notebook org-projectile projectile projectile-codesearch protobuf-mode term-projectile xcscope counsel ivy org org-modern swiper))
 '(tool-bar-mode nil))


;; custom-set-faces is normally added by Custom, but
;; i need one different for X (window-sytem) vs terminal.
;; that means the customize command probably won't work right
;; after this.


(if window-system
    (custom-set-faces
     '(default ((t (:foreground "yellow" :background "black" :font "pfk13"))))
                                        ; i dont know why this doesnt work!
                                        ; '(bold ((t (:font "pfk13bold"))))
     '(bold ((t (:font "pfk13" :foreground "white"))))
     '(bold-italic ((t (:inherit bold))))
     '(button ((t (:inherit default))))
     '(cursor ((t (:background "white" :foreground "red"))))
     '(dired-flagged ((t (:inherit default :foreground "red"))))
     '(dired-mark ((t (:inherit default :foreground "red"))))
     '(dired-marked ((t (:inherit default :foreground "green"))))
     '(dired-symlink ((t (:inherit default :foreground "pink"))))
     '(dired-warning ((t (:inherit default :foreground "red"))))
     '(error ((t (:inherit bold :foreground "red"))))
     '(fixed-pitch ((t (:inherit default))))
     '(font-lock-builtin-face ((t (:inherit default :foreground "LightSteelBlue"))))
     '(header-line ((t (:inherit mode-line :background "grey20" :foreground "grey90" :box nil))))
     '(hi-black-b ((t (:inherit default))))
     '(highlight ((t (:inherit bold :background "darkolivegreen"))))
     '(isearch ((t (:inherit default :background "palevioletred2" :foreground "brown4"))))
     '(italic ((t (:inherit default))))
     '(ivy-minibuffer-match-face-1 ((t (:inherit default :background "#555555"))))
     '(ivy-minibuffer-match-face-2 ((t (:inherit default :background "#777777"))))
     '(ivy-minibuffer-match-face-3 ((t (:inherit default :background "#7777ff"))))
     '(ivy-minibuffer-match-face-4 ((t (:inherit default :background "#8a498a"))))
     '(lazy-highlight ((t (:inherit default :background "paleturquoise4"))))
     '(magit-process-ng ((t (:inherit default :foreground "red"))))
     '(magit-process-ok ((t (:inherit default :foreground "green"))))
     '(match ((t (:inherit bold :background "RoyalBlue3"))))
     '(menu ((t (:inherit default))))
     '(minibuffer-prompt ((t (:inherit default :foreground "cyan"))))
     '(mode-line ((t (:inherit default :background "grey75" :foreground "black" :box (:line-width -1 :style released-button)))))
     '(mode-line-buffer-id ((t (:inherit bold :foreground "black"))))
     '(mode-line-emphasis ((t (:inherit bold))))
     '(mode-line-highlight ((t (:inherit bold :box (:line-width 2 :color "grey40" :style released-button)))))
     '(mode-line-inactive ((t (:inherit default :background "grey30" :foreground "grey80" :box (:line-width -1 :color "grey40")))))
     '(scroll-bar ((t (:background "grey85" :foreground "red"))))
     '(success ((t (:inherit bold :foreground "green"))))
     '(swiper-match-face-1 ((t (:inherit default :background "#555555"))))
     '(swiper-match-face-2 ((t (:inherit default :background "#777777"))))
     '(swiper-match-face-3 ((t (:inherit default :background "#7777ff"))))
     '(swiper-match-face-4 ((t (:inherit default :background "#8a498a"))))
     '(git-gutter:separator ((t (:font "pfk13" :foreground "cyan"    ))))
     '(git-gutter:modified  ((t (:font "pfk13" :foreground "magenta" ))))
     '(git-gutter:added     ((t (:font "pfk13" :foreground "green"   ))))
     '(git-gutter:deleted   ((t (:font "pfk13" :foreground "red"     ))))
     '(git-gutter:unchanged ((t (:font "pfk13" :foreground "yellow"  ))))
     '(warning ((t (:inherit bold :foreground "pink")))))

; if terminal mode needs some custom settings, put them here
; in the 'else' block of this 'if'.

  )
