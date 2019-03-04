-- find the combinations of letters which have the most anagrams
import qualified Data.Map as M
import Data.List(words,sortBy,sort,groupBy,map,intercalate)
import Text.Printf(printf)
import System.Environment(getEnv)

type Anagrams = (Int,String,[String])

printer :: Anagrams -> IO ()
printer (len,sortword,words) = putStrLn $
  printf "%2d %-12s %s" len sortword (intercalate "," words)

sortRevLength :: (Int,a,b) -> (Int,a,b) -> Ordering
sortRevLength (l1,_,_) (l2,_,_) = compare l2 l1

topAnasSort :: [String] -> [Anagrams]
topAnasSort wrds = 
  map ( \(l,x,y)->(l, x, sort y) ) $
  sortBy sortRevLength $
  map (\l -> (length l, fst (head l), map snd l)) $
  groupBy (\x y -> fst x == fst y) $
  sortBy (\(x,_) (y,_) -> compare x y) $
  map (\x -> (sort x, x)) $ wrds

topAnasMap :: [String] -> [Anagrams]
topAnasMap wrds = 
  map (\(l,x,y)->(l,x,reverse y)) $
  sortBy sortRevLength $
  map (\l -> ((length$snd l),(fst l),(snd l)) ) $
  M.toList $
  M.fromListWith (++) (map (\w -> (sort w,[w])) wrds)

-- word list is from 
-- https://raw.githubusercontent.com/dolph/dictionary/master/enable1.txt  
main :: IO () 
main = do
  home <- getEnv "HOME"
  let filename = home ++ "/Downloads/enable1.txt"
  txt <- readFile filename
  let wrds = words txt
  printer (1, "letters", ["anagrams_sort"])
  mapM_ printer $ take 10 $ topAnasSort wrds
  putStrLn ""
  printer (2, "letters", ["anagrams_map"])
  mapM_ printer $ take 10 $ topAnasMap wrds
