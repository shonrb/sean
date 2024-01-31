import Prs
import Rntm
import Exp
import System.Environment (getArgs)

zipIterate :: (a -> b -> a) -> a -> [b] -> [a]
zipIterate f s vals = case vals of
  []       -> []
  (v : vs) -> (n : zipIterate f n vs) where n = f s v

dumpOutput :: Runtime -> IO ()
dumpOutput = mapM_ putStrLn . output

runLines :: [Expression] -> IO ()
runLines = mapM_ dumpOutput . zipIterate rtUpdate rtNew 

main :: IO ()
main = do
  args <- getArgs 
  case args of 
    [] -> putStrLn "An input file is required"
    (file : []) -> do
      contents <- readFile file
      case parseSource contents of
        Left msg    -> putStrLn $ "Parse error: " ++ msg
        Right lines -> runLines lines
    _ -> putStrLn "Too many input files"

