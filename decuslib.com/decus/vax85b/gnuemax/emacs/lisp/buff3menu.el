;; Buffer menu main function and support functions. 
;; Copyright (C) 1985 Richard M. Stallman. 
 
;; This file is part of GNU Emacs. 
 
;; GNU Emacs is distributed in the hope that it will be useful, 
;; but without any warranty.  No author or distributor 
;; accepts responsibility to anyone for the consequences of using it 
;; or for whether it serves any particular purpose or works at all, 
;; unless he says so in writing. 
 
;; Everyone is granted permission to copy, modify and redistribute 
;; GNU Emacs, but only under the conditions described in the 
;; document "GNU Emacs copying permission notice".   An exact copy 
;; of the document is supposed to have been given to you along with 
;; GNU Emacs so that you can know how you may redistribute it all. 
;; It should be in a file named COPYING.  Among other things, the 
;; copyright notice and this notice must be preserved on all copies. 
 
 
 
; Put buffer *Buffer List* into proper mode right away 
; so that from now on even list-buffers is enough to get a buffer menu. 
 
(defvar Buffer-menu-mode-map nil "") 
 
(defun Buffer-menu-mode () 
  "Major mode for editing a list of buffers. 
Each line describes one of the buffers in Emacs. 
Letters do not insert themselves; instead, they are commands. 
m -- mark buffer to be displayed. 
q or C-Z -- select buffer of line dot is on. 
  Also show buffers marked with m in other windows. 
1 -- select that buffer in full-screen window. 
2 -- select that buffer in one window, 
  together with buffer selected before this one in another window. 
~ -- clear modified-flag on that buffer. 
s -- mark that buffer to be saved. 
d or k or C-D or C-K -- mark that buffer to be killed. 
x -- kill or save marked buffers. 
u -- remove all kinds of marks from current line. 
Delete -- back up a line and remove marks." 
  (kill-all-local-variables) 
  (use-local-map Buffer-menu-mode-map) 
  (setq truncate-lines t) 
  (setq major-mode 'Buffer-menu-mode) 
  (setq mode-name "Buffer Menu")) 
 
(save-excursion 
  (setq Buffer-menu-mode-map (make-keymap)) 
  (suppress-keymap Buffer-menu-mode-map t) 
  (define-key Buffer-menu-mode-map "q" 'Buffer-menu-select) 
  (define-key Buffer-menu-mode-map "\^z" 'Buffer-menu-select) 
  (define-key Buffer-menu-mode-map "2" 'Buffer-menu-2-window) 
  (define-key Buffer-menu-mode-map "1" 'Buffer-menu-1-window) 
  (define-key Buffer-menu-mode-map "s" 'Buffer-menu-save) 
  (define-key Buffer-menu-mode-map "d" 'Buffer-menu-kill) 
  (define-key Buffer-menu-mode-map "k" 'Buffer-menu-kill) 
  (define-key Buffer-menu-mode-map "\^d" 'Buffer-menu-kill) 
  (define-key Buffer-menu-mode-map "\^k" 'Buffer-menu-kill) 
  (define-key Buffer-menu-mode-map "x" 'Buffer-menu-execute) 
  (define-key Buffer-menu-mode-map " " 'next-line) 
  (define-key Buffer-menu-mode-map "\177" 'Buffer-menu-backup-unmark) 
  (define-key Buffer-menu-mode-map "~" 'Buffer-menu-not-modified) 
  (define-key Buffer-menu-mode-map "?" 'describe-mode) 
  (define-key Buffer-menu-mode-map "u" 'Buffer-menu-unmark) 
  (define-key Buffer-menu-mode-map "m" 'Buffer-menu-mark)) 
 
(defvar Buffer-menu-buffer-column nil "") 
 
(defvar Buffer-menu-size-column nil "") 
 
(defun Buffer-menu-name () 
  "Return name of buffer described by this line of buffer menu." 
  (if (null Buffer-menu-buffer-column) 
      (save-excursion 
       (goto-char (dot-min)) 
       (search-forward "Buffer") 
       (backward-word 1) 
       (setq Buffer-menu-buffer-column (current-column)) 
       (search-forward "Size") 
       (backward-word 1) 
       (setq Buffer-menu-size-column (current-column)))) 
  (save-excursion 
   (beginning-of-line) 
   (forward-char Buffer-menu-buffer-column) 
   (let ((start (dot))) 
     (move-to-column (1- Buffer-menu-size-column)) 
     (skip-chars-forward "^ \t") 
     (skip-chars-backward " \t") 
     (buffer-substring start (dot))))) 
 
(defun buffer-menu (arg) 
  "Make a menu of buffers so you can save, kill or select them. 
With argument, show only buffers that are visiting files. 
Type ? after invocation to get help on commands available within." 
  (interactive "P") 
  (list-buffers arg) 
  (pop-to-buffer "*Buffer List*") 
  (message 
   "Commands: d, s, x; 1, 2, m, u, q; delete; ~;  ? for help.")) 
 
(defun Buffer-menu-mark () 
  "Mark buffer on this line for being displayed by q command." 
  (interactive) 
  (beginning-of-line) 
  (if (looking-at " [-M]") 
      (ding) 
    (delete-char 1) 
    (insert ?>) 
    (forward-line 1))) 
 
(defun Buffer-menu-unmark () 
  "Cancel all requested operations on buffer on this line." 
  (interactive) 
  (beginning-of-line) 
  (if (looking-at " [-M]") 
      (ding) 
    (let ((mod (buffer-modified-p (get-buffer (Buffer-menu-name))))) 
      (delete-char 3) 
      (insert (if mod " * " 
		"   "))) 
    (forward-line 1))) 
 
(defun Buffer-menu-backup-unmark () 
  "Move up and cancel all requested operations on buffer on line above." 
  (interactive) 
  (forward-line -1) 
  (Buffer-menu-unmark) 
  (forward-line -1)) 
 
(defun Buffer-menu-kill () 
  "Mark buffer on this line to be killed by x command." 
  (interactive) 
  (beginning-of-line) 
  (if (looking-at " [-M]") 
      (ding) 
    (delete-char 1) 
    (insert ?K) 
    (forward-line 1))) 
 
(defun Buffer-menu-save () 
  "Mark buffer on this line to be saved by x command." 
  (interactive) 
  (beginning-of-line) 
  (forward-char 1) 
  (if (looking-at "[-M]") 
      (ding) 
    (delete-char 1) 
    (insert ?S) 
    (forward-line 1))) 
 
(defun Buffer-menu-not-modified () 
  "Mark buffer on this line as unmodified (no changes to save)." 
  (interactive) 
  (let ((foo (Buffer-menu-name))) 
    (save-excursion 
     (set-buffer (get-buffer foo)) 
     (set-buffer-modified-p nil)) 
    (save-excursion 
     (beginning-of-line) 
     (forward-char 1) 
     (if (looking-at "\\*") 
	 (progn 
	  (delete-char 1) 
	  (insert ? )))))) 
 
(defun Buffer-menu-execute () (interactive) 
  "Save and/or kill buffers marked with S or K." 
  (Buffer-menu-do-saves) 
  (Buffer-menu-do-kills)) 
 
(defun Buffer-menu-do-saves () 
  (save-excursion 
   (goto-char (dot-min)) 
   (forward-line 1) 
   (while (re-search-forward "^.S" nil t) 
     (let (modp) 
       (save-excursion 
	(set-buffer (Buffer-menu-name)) 
	(save-buffer) 
	(setq modp (buffer-modified-p))) 
       (delete-char -1) 
       (insert (if modp ?* ? )))))) 
 
(defun Buffer-menu-do-kills () 
  (save-excursion 
   (goto-char (dot-min)) 
   (forward-line 1) 
   (let ((buff-menu-buffer (current-buffer))) 
     (while (re-search-forward "^K" nil t) 
       (forward-char -1) 
       (or (eq (get-buffer (Buffer-menu-name)) buff-menu-buffer) 
	   (kill-buffer (Buffer-menu-name))) 
       (if (get-buffer (Buffer-menu-name)) 
	   (progn (delete-char 1) 
		  (insert ? )) 
	 (delete-region (dot) (progn (forward-line 1) (dot)))))))) 
 
(defun Buffer-menu-select () 
  "Select this line's buffer; also display buffers marked with >." 
  (interactive) 
  (let ((buff (Buffer-menu-name)) 
	others) 
    (goto-char (dot-min)) 
    (while (re-search-forward "^>" nil t) 
      (setq others (cons (Buffer-menu-name) others))) 
    (setq others (nreverse (cons buff others))) 
    (delete-other-windows) 
    (let ((split-height-threshhold 10) 
	  (first t)) 
      (while others 
	(if first (switch-to-buffer (car others)) 
	  (pop-to-buffer (car others))) 
	(setq others (cdr others) 
	      first nil))))) 
     
(defun Buffer-menu-1-window () (interactive) 
  "Select this line's buffer, alone, in full screen." 
  (switch-to-buffer (Buffer-menu-name)) 
  (delete-other-windows)) 
 
(defun Buffer-menu-2-window () (interactive) 
  "Select this line's buffer, with previous buffer in second window." 
  (let ((buff (Buffer-menu-name))) 
    (switch-to-buffer (other-buffer)) 
    (pop-to-buffer buff))) 
