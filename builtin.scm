
(define (insert-string buf s)
  (string-for-each (lambda (c) (insert-char buf c)) s))

(define (help)
  (insert-string (current-buffer) "help"))

(define (value-to-string v)
  (call-with-output-string
    (lambda (string-port)
      (display v string-port))))

(define (value-to-string-write v)
  (call-with-output-string
    (lambda (string-port)
      (write v string-port))))

(define (char-at-cursor-to-upper)
  (let* ((b (current-buffer))
         (c (read-char-at-cursor b)))
    (delete-char-at-cursor b)
    (insert-char b c)))

(define (del-to-eol)
  (let ((b (current-buffer)))
    (let more ()
      (let ((c (read-char-at-cursor b)))
        (if (not (eq? c #\newline))
          (begin
            (delete-char-at-cursor b)
            (more)))))))

