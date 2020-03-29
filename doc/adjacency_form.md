
# Adjacency matrix storage

So in order to prevent `young_reduce` from wasting time on manipulations
with Ex objects, I thought that one should store tensors internally in
some more compact form. Since all terms which we compare have the same
tensor structure, the only thing you need to store is a list of indices
and a list of contractions. E.g. `R_{a b c d} R_{e f c d}` could be 
represented as

    a b c d e f c d .

(I am ignoring upper/lower indices here; you can always correct that at
the very end by raising one index of a dummy pair). But this still has
the disadvantage that dummy indices have explicit labels. Better is a
notation in which a dummy contraction is indicated with the position of
the other index in the index position list. For the above example the
indices are numbered

    R_{a b c d} R_{e f c d}
       0 1 2 3     4 5 6 7 

and then you could represent this as

    a b 6 7 e f 2 3

If you want to store this as a list of integers you could use negative
ones to indicate free indices (with their number referring to a table
of free indices, e.g.)

    -1 -2 6 7 -3 -4 2 3
 
    -1: a
    -2: b
    -3: e
    -4: f

and then you do all young projection etc. on this array of integers.


    class AdjEx {
        public:
            using index_t = short;
            using adjform_t = std::vector<index_t>;
    
            /// Construct from a product in standard Ex notation.
            AdjEx(const Ex&); 
            
            /// Product an Ex from the adjacency form, given a list
            /// of free indices.
            Ex toEx() const;
            
            std::vector<Ex> factors; // with indices, so we can lookup types
    };


# Factoring

    A_{m n} * D_{m p} * Q_{n p} + A_{m n} * E_{m p} * F_{p q} * Q_{n q};
    
Does this become better in adj notation? First need to find
intersection of factors, so we can put them in front.

    A_{m n} Q_{n p} D_{m p}       A_{m n} Q_{n q} E_{m p} F_{p q}
    
But this would work fine if we just rename dummies. So just aim for a
command to move common factors to the front

# Example


Perhaps the following is a nicer example to try things out on, as it doesn't
have Riemann's in them. It comes from the computation that shows that

   Tr( F ^ A ^ A ) = - Tr( A ^ F ^ A )                                (*)

where A is a matrix-valued one-form and F is a matrix-valued two-form.
See e.g. Nakahara's "Geometry, Topology and Physics" chapter 10. To
show this, you could write it all out in indices; in Cadabra notation:

    {a,b,c,d}::Indices(group);
    {\mu,\nu,\rho,\sigma}::Indices(spacetime);
    \epsilon^{\mu\nu\rho\sigma}::EpsilonTensor;

    ex1:=F^{a b}_{\mu\sigma} A^{b c}_{\nu} A^{c a}_{\rho} \epsilon^{\mu\sigma\nu\rho}; 
    ex2:=A^{a b}_{\mu} F^{b c}_{\nu\sigma} A^{c a}_{\rho} \epsilon^{\mu\nu\sigma\rho};

    ex:= @(ex1) + @(ex2);

At the moment you can do 

    sort_product(_);
    canonicalise(_);

to make this vanish and prove (*). Now for the Young reduce approach, you would 
write ex1 and ex2 in 'F A A' form, and then the indices for these two expressions 
would be

          0 1 2  3     4 5 6  7 8 9   10 11    12    13  (index position)
  
     ex1: a b mu sigma b c nu c a rho mu sigma nu    rho
     ex2: b c nu sigma a b mu c a rho mu nu    sigma rho
          ============ ====== ======= ==================
              F          A       A       \epsilon

(you'll need to read this with a fixed-width font). 
Without referring to explicit index names, this turns into

     ipos 0 1  2  3 4 5  6 7 8  9 10 11 12 13
  
     ex1: 8 4 10 11 1 7 12 5 0 13  2  3  6  9               (**)
          ========= ====== ====== ===========
              F       A      A      \eps

and similar for ex2. Nice and compact (you obviously only need to store
the tensor structure 'F A A', the line (**) and the numerical coefficient
of this line, '1' in this case).

Now comes the Young projection. In order to take into account that the 
two A tensors are the same, you need to symmetrise in the index blocks 
for these two factors. This makes indices (4,5,6) become (7,8,9) and
the other way around.

  ipos  0 1  2  3 4 5  6 7 8  9 10 11 12 13

   ex1: 8 4 10 11 1 7 12 5 0 13  2  3  6  9  * (1/2)
        5 7 10 11 8 0 13 1 4 12  2  3  9  6  * (1/2)
        ========= ====== ====== ===========
           F         A     A      \eps

Similar for ex2. Now finally apply the 4! anti-symmetrisation on epsilon;
here are two terms:

 ipos  0 1  2  3 4 5  6 7 8  9 10 11 12 13

   ex1: 5 7 10 11 8 0 13 1 4 12  2  3  9  6 *  (1/48)
        5 7 10 11 8 0 13 1 4 12  2  9  3  6 * (-1/48)
        ...
        ========= ====== ====== ===========
           F         A     A      \eps

So after this you have 48 sets of 14 numbers for ex1, and ditto for ex2.
Now do a linear decomposition of these sets, to find that ex1 is minus ex2.
