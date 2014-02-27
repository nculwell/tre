
(define (insert-string s)
  (string-map insert-char s))

(define (help)
  (insert-string "help"))

(define (value-to-string v)
  (call-with-output-string
    (lambda (string-port)
      (display v string-port))))

(define (value-to-string-write v)
  (call-with-output-string
    (lambda (string-port)
      (write v string-port))))

