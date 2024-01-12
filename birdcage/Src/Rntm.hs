module Rntm where

import Exp
import Fnc
import qualified Data.Map.Strict as Map
import Control.Applicative (Alternative (..))
import Data.Maybe

data Runtime = Runtime
  { syms   :: Map.Map String Term 
  , output :: [String]
  }
  deriving Show

rtNew :: Runtime 
rtNew = Runtime 
  { syms = Map.empty
  , output = []
  }

rtUpdate :: Runtime -> Expression -> Runtime
rtUpdate = rtRunExpr . rtReady

rtReady :: Runtime -> Runtime
rtReady rt = rt { output=[] }

rtRunExpr :: Runtime -> Expression -> Runtime
rtRunExpr rt e = case e of
  TermOnly t     -> fst $ reduceTerm rt t
  Assignment s t -> rt' { syms = Map.insert s t' $ syms rt }
    where (rt', t') = reduceTerm rt t

reduceTerm :: Runtime -> Term -> (Runtime, Term)
reduceTerm rt t = case t of 
  Variable s -> (rt, fromMaybe t $ replaceSym rt s)
  Application lhs rhs -> applyTerms rt'' lhs' rhs' where
    (rt', lhs') = reduceTerm rt lhs
    (rt'', rhs') = reduceTerm rt' rhs
  _ -> (rt, t)

replaceSym :: Runtime -> String -> Maybe Term
replaceSym rt s 
  = Map.lookup s (syms rt) <|> Function <$> getFunction s

applyTerms :: Runtime -> Term -> Term -> (Runtime, Term)
applyTerms rt (Function f) rhs = case applyFunc f rhs of
  Pure t   -> reduceTerm rt t
  Prints s -> (rt { output = output rt ++ [s] }, NullTerm)
applyTerms rt lhs rhs = (rt, Application lhs rhs)

