/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2009  Kasper Peeters <kasper.peeters@aei.mpg.de>

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#ifndef xperm_h_
#define xperm_h_

void canonical_perm(int *perm,
	int SGSQ, int *base, int bl, int *GS, int m, int n,
	int *freeps, int fl, int *dummyps, int dl, int ob, int metricQ,
	int *cperm);

void canonical_perm_ext(int *perm, int n,
	int SGSQ, int *base, int bl, int *GS, int m,
	int *frees, int fl,
        int *vds, int vdsl, int *dummies, int dl, int *mQ,
        int *vrs, int vrsl, int *repes, int rl,
	int *cperm);

void inverse(int *p, int *ip, int n);
int onpoints(int point, int *p, int n);
void copy_list(int *list1, int *list2, int n);


#endif
