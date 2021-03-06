;;; News....M
;;; Created Sun Mar 10,1985 at 21:35:01 ads and sundar 
;;;  Should do the point pdl stuff sometime 
;;; finito except pdl.... Sat Mar 16,1985 at 06:43:44 
 
(defconst news-path "/usr/spool/news/") 
 
;;; random headers that we decide to ignore. 
(defconst news-ignored-headers 
   "^via:\\|^mail-from:\\|^origin:\\|^status:\\|^remailed\\|^received:\\|^message-id:\\|^summary-line:\\|^date-received:\\|^control:\\|^references:") 
 
(defvar news-summary-buffer nil "") 
(defvar news-summary-list nil "") 
(defvar news-mode-map nil "") 
(defvar news-read-first-time-p t "") 
(defvar news-user-group-list nil "") 
(defvar news-current-news-group nil "") 
(defvar news-current-group-begin nil "") 
(defvar news-current-group-end  nil "") 
(defvar news-message-filter nil "User specifiable filter function that will be called during formatting of the news file") 
(defvar news-mode-group-string "Starting-Up" "Mode line group name info is held in this varialble") 
(defvar news-list-of-files nil "Global variable in which we store the list of files associated with the current newsgroup") 
 
;;; association list on which we store intermediate group<->article-read  
;;; relation. 
(defvar news-group-article-assoc nil "") 
(defvar news-groups nil "All news groups of which this user is a member") 
 
(defconst news-startup-file "$HOME/.newsrc" "Contains ~/.newsrc") 
(defvar news-current-message-number 0 "Displayed Article Number") 
(defvar news-total-current-group 0 "Total no of messages in group") 
 
(defconst news-inews-program "/usr/local/inews" "Function to postnews.") 
 
(defvar news-unsubscribe-groups () "") 
(defvar news-temp-buffer "*Group-List*" "") 
(defvar news-point-pdl () "list of visited news messages.") 
(defvar news-no-jumps-p t) 
(defvar news-buffer () "Buffer into which news files are read.") 
 
(defun news-set-minor-modes () 
  "creates a minor mode list that has group name, total articles, 
and attribute for current article." 
  (setq minor-modes (list (cons 'foo 
				(concat news-current-message-number 
					"/" 
					news-total-current-group 
					(news-get-attribute-string)))))) 
 
(defun news-get-attribute-string () 
  "Creates a string of the current article attributes. 
We only get deleted or answered attributes for now" 
  (let ((dot-min-save (dot-min)) 
	(dot-max-save (dot-max)) 
	pos start) 
    (widen) 
    (re-search-backward "" 1 t) 
    (setq start (dot)) 
    (re-search-forward ",," dot-max-save t) 
    (setq pos (dot)) 
    (setq deleted (re-search-backward "deleted" start t)) 
    (goto-char pos) 
    (setq answered (re-search-backward "answered" start t)) 
    (narrow-to-region dot-min-save dot-max-save) 
    (goto-char dot-min-save) 
    (concat (if deleted ",Deleted" "") 
	    (if answered ",Answered" "")))) 
 
(defun news-set-message-counters () 
  "Scan through current news-groups filelist to figure out how many messages 
are there. Set counters for use with minor mode display." 
    (if (null news-list-of-files) 
	(setq news-current-message-number 0))) 
 
(if news-mode-map 
    nil 
  (setq news-mode-map (make-keymap)) 
  (suppress-keymap news-mode-map) 
  (define-key news-mode-map "." 'beginning-of-article) 
  (define-key news-mode-map " " 'scroll-up) 
  (define-key news-mode-map "\177" 'scroll-down) 
  (define-key news-mode-map "n" 'news-next-message) 
  (define-key news-mode-map "p" 'news-previous-message) 
  (define-key news-mode-map "j" 'news-goto-message) 
  (define-key news-mode-map "+" 'news-plus-n-messages) 
  (define-key news-mode-map "-" 'news-minus-n-messages) 
  (define-key news-mode-map "q" 'news-quit) 
  (define-key news-mode-map "e" 'news-exit) 
  (define-key news-mode-map "J" 'news-goto-news-group) 
  (define-key news-mode-map "N" 'news-next-group) 
  (define-key news-mode-map "P" 'news-previous-group) 
  (define-key news-mode-map "l" 'news-list-news-groups) 
  (define-key news-mode-map "?" 'describe-mode) 
  (define-key news-mode-map "g" 'news-get-new-news) 
  (define-key news-mode-map "m" 'news-post-news) 
  (define-key news-mode-map "r" 'news-reply-to-article) 
  (define-key news-mode-map "s" 'news-save-item-in-file) 
  (define-key news-mode-map "t" 'news-show-all-headers) 
  (define-key news-mode-map "u" 'news-unsubscribe-group) 
  (define-key news-mode-map "U" 'news-unsubscribe-any-group) 
  (define-key news-mode-map "h" 'news-summary)) 
 
(defun news-mode () 
  "News Mode is used by M-x news for editing News files. 
All normal editing commands are turned off. 
Instead, these commands are available: 
 
.	Move dot to front of this news article (same as Meta-<). 
Space	Scroll to next screen of this news article. 
Delete  Scroll down previous page of this news article. 
n	Move to Next news article. 
p	Move to Previous news article. 
j	Jump to news article specified by numeric position. 
+	goto current article plus count. 
-	goto current article minus count. 
J       Jump to news group. 
N       Goto next news group. 
P       Goto previous news group. 
l       List all the news groups with current-status. 
?       Print this help message. 
q	Quit without updating .newsrc file. 
e	Exit updating .newsrc file. 
s	Save the current article in the named file.(append if file exists) 
g	Move new mail from /usr/spool/mail or mbox into this file. 
m	Mail a news article.  Same as C-x 4 m. 
r	Reply to this news article.  Like m but initializes some fields. 
t	Show all the headers this news article originally had. 
u       Unsubscribe from current newsgroup. 
U       Unsubscribe from any newsgroup. 
h	Show headers buffer, with a one line summary of each news article." 
  (interactive) 
  (kill-all-local-variables) 
  (make-local-variable 'news-summary-buffer) 
  (make-local-variable 'news-summary-list) 
  (make-local-variable 'news-read-first-timep-p) 
  (make-local-variable 'news-current-news-group) 
  (make-local-variable 'news-current-group-begin) 
  (make-local-variable 'news-current-group-end) 
  (make-local-variable 'news-current-message-number) 
  (make-local-variable 'news-total-current-group) 
  (make-local-variable 'news-point-pdl) 
  (setq news-current-news-group "??") 
  (setq news-current-group-begin 0) 
  (setq news-current-group-end 0) 
  (setq news-current-message-number 0) 
  (setq major-mode 'news-mode) 
  (setq mode-name "NEWS") 
  (news-set-mode-line) 
  (set-syntax-table text-mode-syntax-table) 
  (use-local-map news-mode-map) 
  (setq news-read-first-time-p t) 
  (setq news-group-article-assoc ()) 
  (setq local-abbrev-table text-mode-abbrev-table)) 
 
;;; replace all occurance of . with / it net.unix-wizards becomes 
;;; net/unix-wizards. etc. 
 
(defun replace-dot-with-slash (string) 
  "replace occurences of . with / in string" 
  (let ((index ())) 
   (while (setq index (string-match "\\." string)) 
     (aset string index (string-to-char "/"))) 
   string)) 
 
(defun remove-dash-between-numbers ( number-string ) 
  (let ((index (string-match "-" number-string))) 
    (if (not (null index)) 
	(list (string-to-int (substring number-string 0 index)) 
	      (string-to-int (substring number-string (1+ index)))) 
      (let ((num (if number-string 
		     (string-to-int number-string) 
		   1))) 
	(list num num))))) 
 
(defun news-get-new-news () 
  "Get netnews if there is any for the current user." 
  (interactive) 
  (setq news-group-article-assoc ()) 
  (setq news-user-group-list ()) 
  (message "Looking up .newsrc file...") 
  (let ((file (substitute-in-file-name news-startup-file)) 
	(temp-user-groups ())) 
    (save-excursion 
     (let((newsrcbuf (find-file-noselect file)) 
	  start end endofline) 
       (set-buffer newsrcbuf) 
       (goto-char 0) 
       (while (re-search-forward ": " nil t) 
	 (setq end (- (dot) 2)) 
	 (beginning-of-line) 
	 (setq start (dot)) 
	 (end-of-line) 
	 (setq endofline (dot)) 
	 (let ((temp-var 
		(replace-dot-with-slash (buffer-substring start end)))) 
	   (setq news-group-article-assoc 
		 (cons 
		  (cons temp-var 
			(remove-dash-between-numbers 
			 (buffer-substring (+ end 2) endofline))) 
		  news-group-article-assoc)) 
	   (setq temp-user-groups (cons temp-var temp-user-groups)))))) 
    (message "Loading up .... ") 
    (switch-to-buffer news-buffer) 
    (setq news-user-group-list (nreverse temp-user-groups)) 
    (setq temp-user-groups news-user-group-list) 
    (setq news-current-news-group (car temp-user-groups)) 
    (while (and temp-user-groups 
		(not (news-read-files-into-buffer (car temp-user-groups)))) 
      (setq temp-user-groups (cdr temp-user-groups)) 
      (setq news-current-news-group (car temp-user-groups))) 
    (message "Loading up .... done. ") 
    (if (null temp-user-groups) 
	(message "No news is good news ... ")))) 
 
(defun news-list-news-groups () 
  (interactive) 
  (if (null news-user-group-list) 
      (message "No user groups read yet?") 
    (progn 
     (setq mode-line-format "--%%--[q: to goback, space: scroll-forward, delete:scroll-backward] %M --%--") 
     (local-set-key " " 'scroll-up) 
     (local-set-key "\177" 'scroll-down) 
     (local-set-key "q" 'news-get-back) 
     (goto-char 0) 
     (save-excursion 
     (erase-buffer) 
     (insert "News Group        Msg No.       News Group        Msg No.\n") 
     (insert "-------------------------       -------------------------\n") 
     (let ((temp news-user-group-list) 
	   (mode 1)) 
       (while temp 
	 (let ((item (assoc (car temp) news-group-article-assoc))) 
	   (insert (replace-slash-with-dot (car item))) 
	   (if (zerop mode) 
	       (setq mode (1+ mode)) 
	     (setq mode (1- mode))) 
	   (if (zerop mode) 
	       (indent-to 20)			     
	     (indent-to 52)) 
	   (insert (int-to-string (caddr item))) 
	   (if (zerop mode) 
	       (indent-to 33) 
	       (insert "\n")) 
	   (setq temp (cdr temp))))))))) 
 
(defun news-get-back () 
  (interactive) 
  (erase-buffer) 
  (local-set-key "q" 'news-quit) 
  (news-set-mode-line) 
  (news-read-in-file (concat news-path news-current-news-group "/" 
			     (int-to-string news-current-message-number)))) 
 
 
(defmacro caddr (x) (list 'car (list 'cdr (list 'cdr x)))) 
 
(defun get-pruned-list-of-files (group end-file-no) 
  "given a news group it does an ls to give all files in the news group. the arg must be in slashified format." 
  (let ((file-directory (concat news-path group)) 
	(list-files ()) 
	start end) 
    (save-excursion 
     (switch-to-buffer "*Group-List*") 
     (erase-buffer) 
     (if (file-directory-p file-directory) 
	 (call-process "/bin/ls" nil (get-buffer "*Group-List*") nil 
		       file-directory)) 
     (goto-char 1) 
     (while (re-search-forward "^[0-9]+\n" nil t) 
       (previous-line 1) 
       (beginning-of-line) 
       (setq start (dot)) 
       (end-of-line) 
       (setq end (dot)) 
       (setq list-files 
	     (cons (buffer-substring start end) list-files)) 
       (re-search-forward "\n" nil t))) 
    (setq news-current-group-end (car list-files)) 
    (setq news-list-of-files (nreverse list-files)) 
    (if (null news-list-of-files) 
	 nil 
      (progn 
       (while (and news-list-of-files 
		   (<= (string-to-int (car news-list-of-files)) end-file-no)) 
	 (setq news-list-of-files (cdr news-list-of-files))) 
       news-list-of-files)))) 
 
;; Mode line hack 
(defun replace-slash-with-dot (string) 
  (catch 'news-exit-dot 
    (let ((oindex 0) 
	  (new-str "")) 
      (while t 
	(let ((index (string-match "/" string oindex))) 
	  (if (null index) 
	      (throw 'news-exit-dot  
		     (concat new-str (substring string oindex))) 
	    (progn 
	     (setq new-str  
		  (concat new-str 
			  (substring string oindex index) 
			  ".")) 
	     (setq oindex (1+ index))))))))) 
	 
(defun news-set-mode-line () 
  (setq mode-line-format  
         (concat "--%1*%1*-News(1): (" 
           (replace-slash-with-dot news-current-news-group) 
           "),Mesg: [" 
           (cond ((numberp news-current-message-number) 
                    (int-to-string news-current-message-number)) 
                 (t "??")) 
           "/" 
           news-current-group-end 
	   "]" 
           " %M ----%3p-%-"))) 
 
(defmacro cadr (x) (list 'car (list 'cdr x))) 
 
;; this also seems to be working... 
(defun news-read-files-into-buffer (group) 
  "main function called to read a groups files into a buffer" 
  (let* ((files-start-end (cdr (assoc group news-group-article-assoc))) 
	 (start-file-no (car files-start-end)) 
	 (end-file-no (cadr files-start-end))) 
 
    (get-pruned-list-of-files group end-file-no) 
 
    (setq news-current-news-group group) 
 
    (if (null news-list-of-files) 
	(progn 
	 (erase-buffer) 
	 (setq news-current-group-end end-file-no) 
	 (setq news-current-group-begin end-file-no) 
	 (setq news-current-message-number end-file-no) 
	 (insert "No New Articles in "  
           (replace-slash-with-dot group) " group.")) 
      (progn 
       (setq news-current-group-begin (string-to-int 
				       (car news-list-of-files))) 
       (setq news-point-pdl (list files-start-end)) 
       (if (> (string-to-int (car news-list-of-files)) end-file-no) 
	   (rplaca (cdar news-point-pdl) (string-to-int 
					  (car news-list-of-files)))) 
       (setq news-current-message-number  
	     (string-to-int (car news-list-of-files))) 
       (setq news-current-group-end (string-to-int  
				     news-current-group-end)) 
       (news-set-message-counters) 
       (news-set-mode-line) 
       (news-read-in-file (concat news-path group "/" 
				  (car news-list-of-files))) 
       t)))) 
 
(defun news-goto-news-group (gp) 
  "Takes a string and goes to that news group" 
   (interactive "sNewsGroup: ") 
   (message "Jumping to news group %s ..." gp) 
   (news-go-to-news-group gp) 
   (message "Jumping to news group %s ... done." gp)) 
 
(defmacro cadar (x) 
  (list 'car 
	(list 'cdr 
	      (list 'car x)))) 
		     
;; this is working fine... 
(defun news-go-to-news-group (gp) 
  "Takes a string argument which is the name of the newsgroup to goto." 
   (let ((grp (replace-dot-with-slash gp))) 
     (let ((grpl (assoc grp news-group-article-assoc))) 
       (if grpl 
	 (progn  
	  (news-update-message-read news-current-news-group 
				    (cadar news-point-pdl)) 
	  (news-read-files-into-buffer  (car grpl)) 
	  (setq news-current-news-group (car grpl)) 
	  (news-set-mode-line)) 
       (message "Non-existing newsgroup?"))))) 
 
(defun news-go-to-message (arg) 
  "goes to the article ARG in current newsgroup" 
  (let ((file (concat news-path news-current-news-group "/" 
			      arg))) 
  (if (and (not (null arg)) 
	   (file-exists-p file)) 
      (let ((xx (string-to-int arg))) 
       (if (= xx (1+ (cadar news-point-pdl))) 
	   (rplaca (cdar news-point-pdl) xx)) 
       (setq news-current-message-number xx) 
       (news-read-in-file file) 
       (news-set-mode-line)) 
    (message "article no:%s nonexistent?" arg)))) 
 
(defun news-goto-message (arg) 
  "Go to the article number ARG in current newsgroup." 
  (interactive "p") 
  (news-go-to-message arg)) 
 
(defmacro caadr (x) 
  (list 'car 
	(list 'car 
	      (list 'cdr x)))) 
 
(defun news-move-to-message (arg) 
   "given arg move forward or backward within the news group" 
   (let ((no (+ arg news-current-message-number))) 
     (if (or (< no news-current-group-begin)  
	     (> no news-current-group-end)) 
	 (if (= arg 1) 
	     (progn 
	      (message "Moving to next news group ... ") 
	      (news-next-group) 
	      (message "Moving to next news group ... done.")) 
	   (if (= arg -1) 
	       (progn 
		(message "Moving to previous news group ... ") 
		(news-reverse-read-files-into-buffer 
		 (caadr (news-get-motion-lists 
			 news-current-news-group news-user-group-list))) 
		(message "Moving to previous news group ... done.")) 
	     (message "article out of range?"))) 
       (let ((plist (news-get-motion-lists  
		     (int-to-string news-current-message-number) 
		     news-list-of-files))) 
	 (if (< arg 0) 
	     (news-go-to-message (nth (1- (- arg)) (car (cdr plist)))) 
	   (news-go-to-message (nth (1- arg) (car plist)))))))) 
 
(defmacro caar (x)  
  (list 'car 
	(list 'car x))) 
 
(defun news-next-message () 
  "Goes to the next article within a newsgroup" 
  (interactive) 
  (let ((cg news-current-news-group)) 
    (news-move-to-message 1) 
    (if (not (string-equal cg news-current-news-group)) 
	(if (null news-list-of-files) 
	    (news-next-group))))) 
 
(defun news-previous-message () 
  "Goes to the previous article within a newsgroup" 
  (interactive) 
  (let ((cg news-current-news-group)) 
    (news-move-to-message -1) 
    (if (not (string-equal cg news-current-news-group)) 
	(if (null news-list-of-files) 
	    (news-previous-group))))) 
 
(defun news-plus-n-messages (num) 
  "moves forward n articles within a newsgroup" 
  (interactive "p") 
  (news-move-to-message num)) 
 
(defun news-minus-n-messages (num) 
  "moves backward n articles within a newsgroup" 
  (interactive "p") 
  (news-move-to-message (- num))) 
 
 
(defun news-move-to-group (arg) 
   "given arg move forward or backward to a new newsgroup" 
   (let ((cg news-current-news-group)) 
       (let ((plist (news-get-motion-lists  
		     cg 
		     news-user-group-list))) 
	 (if (< arg 0) 
	     (news-go-to-news-group (nth (1- (- arg)) (cadr plist))) 
	   (news-go-to-news-group (nth arg (car plist))))))) 
       
(defun news-next-group () 
  "moves to the next user group" 
  (interactive) 
  (message "Moving to next group ... ") 
  (news-move-to-group 0) 
  (message "Moving to next group ... done.")) 
 
(defun news-previous-group () 
  "moves to the previous user group" 
  (interactive) 
  (message "Moving to previous group ... ") 
  (news-move-to-group -1) 
  (message "Moving to previous group ... done.")) 
 
(defun news-get-motion-lists (arg listy) 
   "given a msgnumber/group this will return a list of two lists one for moving forward and one for moving backward.." 
   (let ((temp listy) 
	 (result ())) 
     (catch 'out 
     (while temp 
       (if (string-equal (car temp) arg) 
	   (throw 'out (cons (cdr temp) (list result))) 
           (progn 
            (setq result (nconc (list (car temp)) result)) 
	    (setq temp (cdr temp)))))))) 
            
 
;; miscellaneous io routines 
(defun news-read-in-file (filename) 
  (erase-buffer) 
  (let ((start (dot))) 
  (insert-file-contents filename) 
  (news-convert-format) 
  (goto-char start) 
  (forward-line 1) 
  (if (eobp) 
      (message "(Empty file?)") 
      (goto-char start)))) 
 
(defun news-convert-format() 
   (save-excursion 
    (save-restriction 
     (let ((start (dot)) 
	   (end (condition-case err 
				(progn (search-forward "\n\n") (dot)) 
		  (error (message "cant find eoh") nil))) 
	   has-from has-date) 
       (cond (end 
	      (narrow-to-region start end) 
	      (goto-char start) 
	      (setq has-from (search-forward "\nFrom:" nil t)) 
	      (cond ((and (not has-from) has-date) 
		     (goto-char start) 
		     (search-forward "\nDate:") 
		     (beginning-of-line) 
		     (kill-line) (kill-line))) 
	      (news-delete-headers start) 
	      (goto-char start) 
	      )))))) 
 
(defun news-delete-headers(pos) 
  (goto-char pos) 
	      (while (re-search-forward "^Path:\\|^Posting-Version:\\|^Article-I.D.:\\|^Followup-To:\\|^Expires:\\|^Date-Received:\\|^Organization:\\|^References:\\|^Control:\\|^Xref:\\|^Lines:\\|^Posted:\\|^Relay-Version:\\|^Message-ID:" nil t) 
		(beginning-of-line) 
		(delete-region (dot) 
			       (progn (re-search-forward "\n[^ \t]") 
				      (forward-char -1) 
				      (dot))))) 
;;; update read message number 
(defmacro news-update-message-read (ngroup nno) 
  (list 'rplaca 
	(list 'cdr 
	      (list 'cdr 
		    (list 'assoc ngroup 'news-group-article-assoc))) 
		    nno)) 
 
(defun news-quit () 
  "Quit news reading without updating newsrc file." 
  (interactive) 
  (if (y-or-n-p "Quit without updating .newsrc? ") 
      (progn 
       (kill-buffer news-temp-buffer) 
       (kill-buffer news-buffer) 
       (kill-buffer ".newsrc")))) 
 
(defun news-exit () 
  "Quit news reading session and update the newsrc file." 
  (interactive) 
  (if (y-or-n-p "Do you really wanna quit reading news ? ") 
      (progn 
       (news-update-newsrc-file) 
       (kill-buffer news-temp-buffer) 
       (kill-buffer news-buffer) 
       (kill-buffer ".newsrc")))) 
 
(defun replace-dot-for-search (string) 
  (let (index x new-string cindex) 
    (while (setq x (string-match "/" string)) 
      (setq index (cons x index)) 
      (aset string x (string-to-char "."))) 
    (setq x (length string)) 
    (while (setq cindex (car index)) 
      (setq new-string (concat (substring string cindex x) 
			       "\\" 
			       new-string)) 
      (setq x cindex) 
      (setq index (cdr index))) 
    (setq new-string (concat (substring string 0 x) 
			     new-string)) 
    (list new-string string))) 
 
(defmacro cdar (x) 
  (list 'cdr 
	(list 'car x))) 
 
(defun news-update-newsrc-file () 
  "Updates the newsrc file in the users home dir." 
  (let ((group ()) 
	(num-dash-num ()) 
	(group-article ()) 
	(newsrcbuf (find-file-noselect 
		    (substitute-in-file-name news-startup-file)))) 
    (save-excursion 
     (news-update-message-read news-current-news-group 
			       (cadar news-point-pdl)) 
     (switch-to-buffer newsrcbuf) 
     (while news-user-group-list 
       (setq group (car news-user-group-list)) 
       (setq news-user-group-list (cdr news-user-group-list)) 
       (setq group-article (assoc group news-group-article-assoc)) 
       (setq num-dash-num (concat (int-to-string (cadr group-article)) "-" 
				  (int-to-string (caddr group-article)))) 
       (goto-char 0) 
       (if (re-search-forward (concat "^" 
				      (car (replace-dot-for-search group)) 
				      ": ") nil t) 
	   (kill-line nil)) 
       (insert num-dash-num)) 
     (while news-unsubscribe-groups 
       (setq group (car news-unsubscribe-groups)) 
       (setq news-unsubscribe-groups (cdr news-unsubscribe-groups)) 
       (setq group-article (assoc group news-group-article-assoc)) 
       (setq num-dash-num (concat (int-to-string (cadr group-article)) 
				  "-" 
				  (int-to-string (caddr group-article)))) 
       (goto-char 0) 
       (if (re-search-forward (concat "^" 
				      (car (replace-dot-for-search group)) 
				      ": ") nil t) 
	   (progn 
	    (move-to-column (- (current-column) 2)) 
	    (kill-line nil) 
	    (insert "! ") 
	    (insert num-dash-num)))) 
     (save-buffer)))) 
 
(defun news-goto-news-group (gp) 
  "Takes a string argument which is the name of the newsgroup to goto." 
  (interactive "sNewsGroup: ") 
  (let ((grp (replace-dot-with-slash gp))) 
    (let ((grpl (assoc grp news-group-article-assoc))) 
      (if grpl 
	  (progn  
	   (news-read-files-into-buffer  (car grpl)) 
	   (setq news-current-news-group (car grpl)) 
	   (news-set-mode-line)) 
	(message "Non-existing newsgroup?"))))) 
 
(defun news-unsubscribe-any-group (group) 
  "Removes the group from the list of groups that you currently subscribe too." 
  (interactive "sUnsubscribe from group: ") 
  (news-unsubscribe-internal group)) 
 
(defun news-unsubscribe-group () 
  "removes the user from current group" 
  (interactive) 
  (if (y-or-n-p "Do you really want to unsubscribe from this group ? ") 
      (news-unsubscribe-internal news-current-news-group))) 
 
(defun news-unsubscribe-internal (group) 
  (let ((temp-list (assoc group news-group-article-assoc))) 
    (if (not (null temp-list)) 
	(progn 
	 (setq news-unsubscribe-groups (cons (car temp-list) 
					     news-unsubscribe-groups)) 
	 (news-update-message-read group (cadar news-point-pdl)) 
	 (if (string-equal group news-current-news-group) 
	       (news-next-group))) 
      (message "Member-p of %s ==> nil" news-current-news-group)))) 
 
(defun rnews () 
  "Read netnews for groups for which you are a member and add or delete groups. 
You can reply to articles posted and send articles to any group. 
  Type Help m once reading news to get a list of rnews commands." 
  (interactive) 
  (switch-to-buffer (setq news-buffer (get-buffer-create "*news*"))) 
  (setq buffer-read-only t) 
  (news-mode) 
  (set-buffer-modified-p t) 
  (sit-for 0) 
  (message "Getting new net news ... ") 
  (erase-buffer) 
  (news-set-mode-line) 
  (news-get-new-news)) 
 
(defun news-reverse-read-files-into-buffer (group) 
  "main function called to read a groups files into a buffer" 
  (let* ((files-start-end (cdr (assoc group news-group-article-assoc))) 
	 (start-file-no (car files-start-end)) 
	 (end-file-no (cadr files-start-end))) 
 
    (get-pruned-list-of-files group end-file-no) 
 
    (setq news-current-news-group group) 
 
    ;; should be a lot smarter than this if we have to move around correctly. 
    (setq news-point-pdl (list files-start-end)) 
 
    (if (null news-list-of-files) 
	(progn 
	 (erase-buffer) 
	 (setq news-current-message-number end-file-no) 
	 (setq news-current-group-begin end-file-no) 
	 (setq news-current-group-end end-file-no) 
	 (news-set-mode-line) 
	 (insert "No New Articles in " 
           (replace-slash-with-dot group) " group.")) 
      (progn 
       (setq news-current-group-begin 
	     (string-to-int (car news-list-of-files))) 
       (news-read-in-file (concat news-path group "/"  
				  news-current-group-end)) 
       (setq news-current-group-end (string-to-int news-current-group-end)) 
       (setq news-current-message-number news-current-group-end) 
       (news-set-message-counters) 
       (news-set-mode-line) 
       t)))) 
 
(defun news-save-item-in-file (file) 
  "" 
  (interactive "FSave item in file: ") 
  (write-file file)) 
 
(defun news-reply-to-article () 
  "Reply to article in current news group." 
  (interactive) 
  (news-internal-reply)) 
 
(defun news-inews () 
  "Send a news message using inews." 
  (interactive) 
  (setq news-groups-to-send-to (rmail-fetch-field "newsgroups")) 
  (setq news-title (rmail-fetch-field "subject")) 
  (let ((start ())) 
    (set-buffer "*mail*") 
    (goto-char (dot-min)) 
    (search-forward "\n--text follows this line--\n") 
    (setq start (dot)) 
    (message "Posting ..... ") 
    (call-process-region start (dot-max)  
			 news-inews-program nil 0 nil 
			 "-t" news-title 
			 "-n" news-groups-to-send-to) 
    (message "Posting ..... done") 
    (local-set-key "\^z\^z" 'mail-send-and-exit) 
    (local-set-key "\^z\^s" 'mail-send) 
    (kill-buffer "*mail*") 
    (if (eq (next-window (selected-window)) (selected-window)) 
	(switch-to-buffer (other-buffer (current-buffer))) 
      (delete-window)))) 
		        
(defun news-internal-reply () 
  "Reply to the current message. 
Normally include CC: to all other recipients of original message; 
prefix argument means ignore them. 
While composing the reply, use C-z y to yank the original message into it." 
  (let ((just-sender nil)) 
    (news-call-mail-setup 
     (or (rmail-fetch-field "from") 
	 (rmail-fetch-field "reply-to")) 
     (rmail-fetch-field "subject") 
     (let* ((string (rmail-fetch-field "from")) 
	    (stop-pos (string-match "  *at \\|  *@ \\| *(\\| *<" string))) 
       (concat (if stop-pos (substring string 0 stop-pos) string) 
	       "'s message of " 
	       (rmail-fetch-field "date"))) 
     (and (not just-sender) 
	  (progn 
	   (setq cc-list 
		 (rmail-dont-reply-to 
		  (concat (rmail-fetch-field "to") 
			  (progn (setq cc-field 
				       (rmail-fetch-field "cc")) 
				 (if (or (string= cc-field "") 
					 (null cc-field)) 
				     "" 
				   (concat ", " 
					   cc-field)))))) 
	   (if (string= cc-list "") nil 
	     cc-list))) 
     (current-buffer)))) 
 
(defun news-call-mail-setup (to subject in-reply-to cc replybuffer) 
  (pop-to-buffer "*mail*") 
  (mail-mode) 
  (local-set-key "\^z\^z" 'news-inews) 
  (local-set-key "\^z\^s" 'news-inews) 
  (erase-buffer) 
  (mail-setup to subject in-reply-to cc replybuffer)) 
 
 
(defun news-post-news () 
  "post new news." 
  (interactive) 
  (pop-to-buffer "*mail*") 
  (mail-mode) 
  (local-set-key "\^z\^z" 'news-inews) 
  (local-set-key "\^z\^s" 'news-inews) 
  (erase-buffer) 
  (insert "\nNewsgroups: ") 
  (mail-setup () () () () ())) 
 
