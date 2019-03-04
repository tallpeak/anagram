; find the combinations of letters which have the most anagrams
; word list is from 
; https://raw.githubusercontent.com/dolph/dictionary/master/enable1.txt  
(ns anagrams-sort
  (:require [clojure.string :as cs]) )

(defn main []  

(def words
  (let [ home     (System/getenv "HOME")
         filename (str home "/Downloads/enable1.txt")
         words    (->> filename slurp cs/split-lines)
        ] words )
)

;(defn negate [x] (- 0 x))

(def sortwords
  (->> (reduce (fn [acc word]
               (update acc (apply str (sort word)) conj word))
               {}
               words)
       (sort-by #(-> % second count)))
  )

(take 10 (reverse sortwords))
              
)

(time (main))

;; reduce line is equivalent to:
;; (reduce (fn [accumulator word] (update accumulator (sort word) conj word)) {})
;or just (Justin's solution)
(comment 
  (->> "/home/aaron/Downloads/enable1.txt" slurp
       clojure.string/split-lines
       (reduce #(update %1 (sort %2) conj %2) {}) 
       (sort-by (comp count second))
       last last)
)
