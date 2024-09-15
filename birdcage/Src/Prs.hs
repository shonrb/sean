module Prs (parseSource) where

import Exp
import Control.Monad (ap)
import Control.Applicative (Alternative (..), liftA2)
import Data.Char (isSpace)

type ParseResult a = Either String a

newtype Parser a = Parser
  { parse :: String -> ParseResult (a, String)
  }

instance Functor Parser where
  fmap f p = p >>= (return . f)

instance Applicative Parser where
  pure x = Parser $ \s -> Right (x, s)
  (<*>)  = ap

instance Monad Parser where
  return = pure
  p >>= x = Parser $ \s -> case parse p s of
    Right (a, b) -> parse (x a) b
    Left msg     -> Left msg 

instance Alternative Parser where
  empty     = pErr "This shouldn't happen"
  pa <|> pb = Parser $ \s -> case parse pa s of
    Right a -> Right a
    Left _  -> parse pb s

pErr :: String -> Parser a
pErr msg = Parser $ \s -> Left msg

pAny :: Parser Char
pAny = Parser $ \s -> case s of
  []       -> Left "Unexpected end of line"
  (c : cs) -> Right (c, cs)

pPred :: String -> (Char -> Bool) -> Parser Char 
pPred need f = do
  c <- pAny
  if f c
  then return c
  else pErr $ "Expected " ++ need ++ ", got " ++ [c]

pChar :: Char -> Parser Char
pChar c = pPred [c] (==c)

pString :: String -> Parser String
pString = traverse pChar

pEscapedChar :: Parser Char 
pEscapedChar = do
  c <- pAny
  case c of 
    '"'  -> pErr ""
    '\\' -> pAny
    _    -> return c

pStringLit :: Parser Term
pStringLit = fmap StringLit $ pChar '"' *> pZeroPlus pEscapedChar <* pChar '"'
 
pOnePlus :: Parser a -> Parser [a]
pOnePlus p = liftA2 (:) p $ pZeroPlus p

pZeroPlus :: Parser a -> Parser [a]
pZeroPlus p = pOnePlus p <|> return [] 

pSpaces :: Parser String
pSpaces = pZeroPlus $ pPred "whitespace" isSpace

pName :: Parser String
pName = pOnePlus $ pPred "identifier" 
  (\c -> not (isSpace c) && c /= '(' && c /= ')')

pIdentifier :: Parser Term
pIdentifier = Variable <$> pName

pGroup :: Parser Term
pGroup = pChar '(' *> pTerm <* pChar ')' 

pTermBegin :: Parser Term
pTermBegin = pStringLit <|> pIdentifier <|> pGroup 

pTermEnd :: Parser [Term]
pTermEnd = pSpaces *> pSubTerms <|> return []
      
pSubTerms :: Parser [Term]
pSubTerms = liftA2 (:) (pSpaces *> pTermBegin) (pTermEnd <* pSpaces)

foldSubTerms :: [Term] -> Term
foldSubTerms [a] = a
foldSubTerms s   = Application (foldSubTerms (init s)) (last s)

pTerm :: Parser Term
pTerm = fmap foldSubTerms pSubTerms

pAssignLHS :: Parser String
pAssignLHS = pSpaces *> pName <* pSpaces <* pChar '='

pAssignment :: Parser Expression
pAssignment = liftA2 Assignment pAssignLHS pTerm

pExpr :: Parser Expression
pExpr = pAssignment <|> TermOnly <$> pTerm <|> pErr "Couldn't tell you mate"

fullParse :: Parser a -> String -> ParseResult a
fullParse p s = do
  (a, s) <- parse p s
  case s of
    "" -> Right a
    _  -> Left $ "Trailing input: " ++ show s

parseSource :: String -> ParseResult [Expression]
parseSource = sequence . map (fullParse pExpr) . filter (/="") . lines

