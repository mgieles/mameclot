// Program:  MAke ME a CLuster Or Two (MAMECLOT)
// Author:   Mark Gieles 
// Date:     August 2012

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "mameclot.h"

#define pcMyr2kms 0.9778
#define G         0.004499 // pc3 Msun-1 Myr-2
#define PI        3.141592653589793239
#define TWOPI     2.0*PI
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a)) // Needed for dawson routine
#define sign(a)   a/fabs(a) 
#define sqr(x)    pow(x,2.0)
#define cube(x)   pow(x,3.0)

/*************************************************/
void parameter_use()
{
  fprintf(stderr,"\n MAMECLOT: MAke ME a CLuster Or Two\n\n");
  fprintf(stderr," Construct initial conditions of 1 (or 2) cluster(s) for N-body simulations \n\n");
  fprintf(stderr," A 2 cluster system with orbit in the x-y plane is made if the mass ratio q>0 \n");
  fprintf(stderr," Five equilibrium models are available, a cut-off radius can be provided\n");
  fprintf(stderr," The velocities can be isotropic or radially anisotropic (a la Osipkov-Merritt)\n");
  fprintf(stderr," Optional angular momentum by aligning the angular momentum vectors along z\n");
  fprintf(stderr," System is scaled to N-body units with G = M = -4E = 1 (Heggie & Mathieu 1986)\n");
  fprintf(stderr," In the case of a two-body system E = mu*e_orb + E_1 + E_2 (mu = reduced mass) \n");
  fprintf(stderr," The 2-body orbit is computed from input q, e_hat and l_hat (and eta if q<1):\n");
  fprintf(stderr,"        e_hat = e_orb/0.5<sigma^2> \n");
  fprintf(stderr,"        l_hat = l_orb/<r_vir*sigma>\n");
  fprintf(stderr,"        eta => defines the relative radii: r2/r1 = q^eta, for example:\n");
  fprintf(stderr,"               eta=[-0.33/0/0.33/0.5] for equal [t_rh/r_h/rho_h/Sigma_h]\n\n");
  fprintf(stderr," Mass, pos[3], vel[3] are written to standard output\n");
  fprintf(stderr," System diagnostics are written to the standard error\n\n");
  fprintf(stderr," Usage: mameclot [OPTIONS] \n\n");
  fprintf(stderr," Options: \n");
  fprintf(stderr,"          -N Total number of stars [10000]\n");
  fprintf(stderr,"          -m Cluster model [3]\n");
  fprintf(stderr,"             0: Dehnen (1993): gamma=0            =>     core, -4 halo\n");
  fprintf(stderr,"             1: Hernquist (1990)                  =>  -1 cusp, -4 halo\n");
  fprintf(stderr,"             2: Jaffe (1983)                      =>  -2 cusp, -4 halo\n");
  fprintf(stderr,"             3: Henon (1959) \"Isochrone\" sphere   =>     core, -4 halo\n");
  fprintf(stderr,"             4: Plummer (1911)                    =>     core, -5 halo\n");
  fprintf(stderr,"          -q Mass ratio of the two clusters (q=m2/m1) [0] \n");
  fprintf(stderr,"             0: Make 1 cluster\n");
  fprintf(stderr,"             0<q<=1: Make 2 clusters\n");
  fprintf(stderr,"          -e eta: Relative cluster sizes: r2/r1 = q^eta [0.333] \n");
  fprintf(stderr,"          -i IMF [0] \n");
  fprintf(stderr,"             0: Single mass cluster\n");
  fprintf(stderr,"             1: Kroupa (2001) between 0.1 Msun and 100 Msun\n");
  fprintf(stderr,"          -l Angular momentum in z-direction, -, + or x [0] \n");
  fprintf(stderr,"              0: xx (both none)\n");
  fprintf(stderr,"              1: ++ (both positive)\n");
  fprintf(stderr,"              2: +x \n");
  fprintf(stderr,"              3: +- \n");
  fprintf(stderr,"              4: x+ \n");
  fprintf(stderr,"              5: x- \n");
  fprintf(stderr,"             -1: -- (both negative)\n");
  fprintf(stderr,"             -2: -x \n");
  fprintf(stderr,"             -3: -+ \n");
  fprintf(stderr,"          -f Fraction of maximum rotational energy [1] \n");
  fprintf(stderr,"          -a Osipkov-Merritt anisotropy radius in units of r_0 [999] \n");
  fprintf(stderr,"          -c Cut-off radius in units of r_h [20] \n");
  fprintf(stderr,"          -r Physical scale in pc [1] \n");
  fprintf(stderr,"          -p Print potential [false] \n");
  fprintf(stderr,"          -P Compute total potential from all pairs (otherwise analytically) [false] \n");
  fprintf(stderr,"          -S Mass segregation fraction between 0 and 1 (Baumgardt et al. 2008) [0] \n");
  fprintf(stderr,"          -u Upper mass in Msun [100] \n");
  fprintf(stderr,"          -d Distance between 2 clusters in N-body units [20] \n");
  fprintf(stderr,"          -E Dimensionless orbital energy of two-cluster system [0] \n");
  fprintf(stderr,"          -L Dimensionless orbital angular momentum two-cluster system [4]\n");  
  fprintf(stderr,"             <0 : forces a circular orbit\n");  
  fprintf(stderr,"          -s Random seed [0=from clock] \n\n");
  fprintf(stderr," Example 1: Isochrone sphere with a Kroupa mass function and Osipkov-Merrit \n");
  fprintf(stderr,"            anisotropy radius equal to half-mass radius:\n");
  fprintf(stderr,"            (./mameclot -m 3 -a 3.06 -i 1 > snap.tab)2> diag.txt\n\n");
  fprintf(stderr," Example 2: Two Hernquist models with isotropic velocity distribution on a \n");
  fprintf(stderr,"            circular orbit and a mass ratio of 1/3:\n");
  fprintf(stderr,"            (./mameclot -m 1 -q 0.333 -L -1 > snap.tab)2> diag.txt\n\n");

  exit (0);
}

/*************************************************/
void parameter_check(INPUT *parameters){
  char name[6][15] = { "Cored gamma", "Hernquist", "Jaffe", "Isochrone", "Plummer", "Gamma1.5" };
  double minra[6] = {0.315,1.0/sqrt(24.0),0.023,0.874,0.75, 998};  // Minimum anistropy radius for positive DF
  strcpy(parameters->name, name[4]);
  
  // Check if anisotropy can be applied
  if (parameters->ra < minra[parameters->model]){
    fprintf(stderr," *** \n *** Input error: Minimum anistropy radius for %s model = %5.3f * r0 \n *** \n",name[parameters->model],minra[parameters->model]);
    exit (0);
  }

  // Check for maximum Ehat
  if (parameters->Ehat >= 4){
    fprintf(stderr," *** \n *** Input error: Ehat >= 4 : total energy positive \n *** \n");
    exit (0);
  }
  
  // Check q
  if ((parameters->q < 0)||(parameters->q>1)){
    fprintf(stderr," *** \n *** Input error: q must be between 0 and 1 \n *** \n");
    exit (0);
  }

  // Check rcut
  if (parameters->rcut < 5){
    fprintf(stderr," *** \n *** Input error: cut-off radius must be larger than 5r_h \n *** \n");
    exit (0);
  }

  // Check d
  if (parameters->d <= 0){
    fprintf(stderr," *** \n *** Warning: d must be positive \n *** \n");
    exit (0);
  }

  // Check segregation -S
  if ((parameters->fsegr>0)&&(parameters->imftype==0)){
    fprintf(stderr," *** \n *** Input error: mass function required for segregation \n *** \n");
    exit (0);
  }

  if ((parameters->fsegr>1)||(parameters->fsegr<0)){
    fprintf(stderr," *** \n *** Input error: mass segregation fraction needs to be 0-1 \n *** \n");
    exit (0);
  }
  
  // Check frot
  if ((parameters->frot < 0)||(parameters->frot>1)){
    fprintf(stderr," *** \n *** Input error: f_rot must be between 0 and 1 \n *** \n");
    exit (0);
  }

  // Check imftype
  if ((parameters->imftype < 0)||(parameters->imftype>1)){
    fprintf(stderr," *** \n *** Input error: IMF type must be 0 or 1 \n *** \n");
    exit (0);
  }

  // Check upper mass
  if (parameters->mup < 0.5){
    fprintf(stderr," *** \n *** Input error: Upper mass must be > 0.5 Msun \n *** \n");
    exit (0);
  }

  if ((parameters->mup != 100)&&(parameters->imftype==0)){
    fprintf(stderr," *** \n *** Warning: no IMF set, upper mass ignored \n *** \n");
  }

  if (parameters->model <= 2)
    parameters->gamma = parameters->model;
  
  if (parameters->model >= 6){
    fprintf(stderr," *** \n *** Input error: -m %2i : model must be <= 5\n *** \n",parameters->model);
    exit(0);
  }
  strcpy(parameters->name, name[parameters->model]);

  if (parameters->q > 0)
    {
      parameters->Ncl = 2;
      // divide over 2 clusters, not so elegant at the moment
      parameters->N2 = (int)parameters->N*parameters->q/(1.0+parameters->q);
      parameters->N -= parameters->N2;
    }
  // Check phi computatios
  if ((!parameters->doPhi)&&(parameters->Ncl==2)){
      fprintf(stderr," *** \n *** Warning: for binary cluster phi is computed pair-wise \n *** \n");
      parameters->doPhi=true;
  }

  if (parameters->seed==0)
    parameters->seed = (unsigned) time(NULL);
  srand(parameters->seed);
}

/*************************************************/
double myrand()
{
  return rand()/((double)RAND_MAX + 1.0);
}
/*************************************************/
int myrandint(int min, int max) {
  return (rand() % (max-min)) + min;
}


/*************************************************/
void get_args(int argc, char** argv, INPUT *parameters)
{

  // User defined parameters
  parameters->N         = 10000;  
  parameters->N2        = 0;  
  parameters->model     = 3; //  0=Cored gamma-model; 1=Hernquist; 2=Jaffe; 3=Isochrone; 4=Plummer; 

  parameters->print_phi = false;
  parameters->doPhi     = false;
  parameters->spin      = 0;  
  parameters->frot      = 1;  
  parameters->ra        = 999;  
  parameters->imftype   = 0;  // 0=equal mass; 1=Kroupa (2001)
  parameters->mup       = 100.0; //Msun
  parameters->seed      = 0;  // pc  
  parameters->rbar      = 1;  // pc
  parameters->q         = 0;  
  parameters->eta       = 0.333;  
  parameters->d         = 20;  
  parameters->rcut      = 20;  
  parameters->fsegr     = 0;  
  parameters->Ehat      = 0;  
  parameters->Lhat      = 4;  
  
  // Non user defined parameters
  parameters->gamma  = 0; //  Only used for model<=2
  parameters->Ncl    = 1;  

  
  for(int i = 1; i < argc; i++){
    if (argv[i][0] == '-'){
      switch (argv[i][1]){
      case 'N': parameters->N = atof(argv[++i]);
	break;
      case 'q': parameters->q = atof(argv[++i]);
	break;
      case 'c': parameters->rcut = atof(argv[++i]);
	break;
      case 'd': parameters->d = atof(argv[++i]);
	break;
      case 'i': parameters->imftype = atoi(argv[++i]);
	break;
      case 'e': parameters->eta = atof(argv[++i]);
	break;
      case 'l': parameters->spin = atoi(argv[++i]);
	break;
      case 'f': parameters->frot = atof(argv[++i]);
	break;
      case 'a': parameters->ra = atof(argv[++i]);
	break;
      case 'E': parameters->Ehat = atof(argv[++i]);
	break;
      case 'L': parameters->Lhat = atof(argv[++i]);
	break;
      case 's': parameters->seed = atof(argv[++i]);
	break;
      case 'm': parameters->model = atoi(argv[++i]);
	break;
      case 'p': parameters->print_phi = true;
	break;
      case 'r': parameters->rbar = atof(argv[++i]);
	break;
      case 'P': parameters->doPhi = true;
	break;
      case 'S': parameters->fsegr = atof(argv[++i]);
	break;
      case 'u': parameters->mup = atof(argv[++i]);
	break;
      case 'h': parameter_use();
	break;
      case 'H': parameter_use();
	break;
      }
    }  
  }
  parameter_check(parameters);

}

/*************************************************/
void allocate_memory(SYSTEM **system, INPUT *parameters)
{
  STAR *stars = malloc(sizeof(STAR));
  CLUSTER *clusters = malloc(sizeof(CLUSTER));
  *system = malloc(sizeof(SYSTEM));

  (*system)->clusters = calloc(parameters->Ncl, sizeof(*clusters));

  for (int i = 0; i < parameters->Ncl; ++i){
    if (i==0)
      (*system)->clusters[i].stars = calloc(parameters->N, sizeof(*stars));

    // TBD memory for mass segregated system
    
    if (i==1)
      (*system)->clusters[i].stars = calloc(parameters->N2, sizeof(*stars));
  }
}
    

/*************************************************/
void initialize(SYSTEM **system, INPUT parameters)
{     
  // Set properties of the system from input parameters
  (*system)->Ncl = parameters.Ncl;
  (*system)->N = parameters.N;
  (*system)->seed = parameters.seed;
  (*system)->rstar = parameters.rbar;
  (*system)->rfac = 1.0; // rfac and vfac will be updated in case of 2 clusters
  (*system)->vfac = 1.0;
  (*system)->clusters[0].N = parameters.N;
  (*system)->clusters[0].M = 1.0;
  (*system)->clusters[0].id = 0;
  (*system)->clusters[0].imftype = parameters.imftype;
  (*system)->clusters[0].print_phi = parameters.print_phi;
  (*system)->clusters[0].doPhi = parameters.doPhi;
  (*system)->clusters[0].mup = parameters.mup;
  (*system)->clusters[0].rvir = 1.0;
  (*system)->clusters[0].rcut = parameters.rcut;
  (*system)->clusters[0].ra = parameters.ra;
  (*system)->clusters[0].vrms = 1.0/sqrt(2.0);
  (*system)->clusters[0].model = parameters.model;
  (*system)->clusters[0].gamma = parameters.gamma;
  (*system)->clusters[0].fsegr = parameters.fsegr;


  strcpy((*system)->clusters[0].name, parameters.name);

  if ((fabs(parameters.spin)>=1)&&(fabs(parameters.spin)<=3))
    {
      (*system)->clusters[0].spin = sign(parameters.spin);
      (*system)->clusters[0].frot = parameters.frot;
    }
  if (parameters.spin>=4)
    (*system)->clusters[0].spin = 0;

  double q = parameters.q;  
  if (q > 0)
    {
      double b=0.0;
      double vrel, msig2, mrsig;
      double Ehat, Lhat, Lhatmax=999.0, Ehatmin;

      double d = parameters.d;
      double eta = parameters.eta;
      double f = 1.0 + q;
      double g = 1.0 + pow(q,1.5+0.5*parameters.eta);
      double h = 1.0 + pow(q,2.0-parameters.eta);    

      double m1 = 1.0/f;
      double m2 = 1.0 - m1;
      
      double mu = m1*m2;

      // Some variables that may change 
      Ehat = parameters.Ehat;
      Lhat = parameters.Lhat;         
      
      // Set specifics of the two-body orbit 
      msig2 = 0.5/(1.0 - mu*Ehat);

      // At this stage it is already ensured that Ehat < 4 (Ehat = 4 has positive energy)

      // Check 1: if negative Lhat force cirular orbit based on d and reset Ehat and Lhat
      if (Lhat < 0)
	{
	  b = d;	  
	  vrel = sqrt(1.0/b);

	  // Mean sig2 follows from:       E =     mu*E_orb + E1 + E2
	  //                        => -0.25 = -0.5mu*vrel2 -0.5<sig2>
	  msig2 = 0.5 - mu*sqr(vrel); 
	  Ehat = -sqr(vrel)/msig2;	  
	  Lhat = 4.0/sqrt(-Ehat) * sqr(f)/(2.0*g) *sqrt(f/h);

	  fprintf(stderr," *** \n");
	  fprintf(stderr," *** Info: Lhat < 0 => circular orbit, recompute Ehat and Lhat\n");	  
	  fprintf(stderr," ***       b = d = %6.2f and v_rel = 1/sqrt(b) = %8.4f\n",b,vrel);
	  fprintf(stderr," *** \n");	  
	}

      // Check 2: if Ehat < Ehatmin (v=0 at distance d), Ehat = Ehatmin, Lhat = 0
      Ehatmin = 4.0/(4.0*mu - d);
      if (Ehat < Ehatmin)
	{
	  Ehat = Ehatmin;
	  Lhat = 0.0;

	  fprintf(stderr," *** \n");
	  fprintf(stderr," *** Warning: Ehat < Ehatmin = %7.2f =>; Ehat = Ehatmin and Lhat = 0 \n",Ehatmin);
	  fprintf(stderr," *** \n");	  
	}

      // Check 3: if Lhat > Lhatmax(d, Ehat) ; then Lhat = Lhatmax
      //          Lhatmax is based on the angular momentum for d = b (i.e. no radial v component)
      //          Note that if circular orbit is set above, Lhat = Lhatmax, by definition
      Lhatmax = (sqr(f)/g) * sqrt(f*d/h) * sqrt( (d-4*mu)*Ehat + 4)/(1-mu*Ehat);
      if ((float)Lhat > (float)Lhatmax){
	Lhat = Lhatmax;

	fprintf(stderr," *** \n");
	fprintf(stderr," *** Warning: Lhat > Lhatmax = %7.2f => Lhat = Lhatmax \n",Lhatmax);
	fprintf(stderr," *** \n");
      }

      // Compute msig2 and vrel from Ehat values
      // Note that in case of circular orbit set above, <sig2>, b and vrel are redefined, is ok
      msig2 = 0.5/(1.0 - mu*Ehat); 
      vrel = sqrt(Ehat*msig2 + 2.0/d);

      // Get values for individual components
      double sig1 = sqrt(msig2*f/h);
      double sig2 = sqrt(pow(q,1.0-eta))*sig1;
      
      double r1 = 0.5*m1/sqr(sig1);
      double r2 = 0.5*m2/sqr(sig2);      

      mrsig = r1*sig1*h/f;
      mrsig = 0.5*g/sqr(f)*sqrt(h/(f*msig2));

      // Only compute b for v_rel > 0
      if (vrel > 0)
	b = Lhat*mrsig/vrel;

      double lambda_orb = (mu*Lhat*mrsig)*sqrt(0.25);
      
      double dx = 0.0;
      double acc = 0.0;
      double tenc = 0.0;

      if (b<d)
	{
	  dx = sqrt(sqr(d) - sqr(b));
	  acc = dx/cube(d);
	  tenc = -vrel/acc + sqrt(sqr(vrel) + 2.0*acc*dx)/acc;
	}

      (*system)->N = parameters.N + parameters.N2;
      (*system)->clusters[1].N = parameters.N2;
      (*system)->d = parameters.d;
      (*system)->clusters[1].imftype = parameters.imftype;
      (*system)->clusters[1].doPhi = parameters.doPhi;
      (*system)->clusters[1].mup = parameters.mup;
      (*system)->clusters[1].id = 1;
      (*system)->clusters[1].model = parameters.model;
      (*system)->clusters[1].gamma = parameters.gamma;
      (*system)->clusters[1].ra = parameters.ra;
      (*system)->clusters[1].rcut = parameters.rcut;
      (*system)->clusters[1].fsegr = parameters.fsegr;

      if ((fabs(parameters.spin)==1)||(parameters.spin==4))
	(*system)->clusters[1].spin = sign(parameters.spin);
      if ((fabs(parameters.spin)==3)||(parameters.spin==5))
	(*system)->clusters[1].spin = -1.0*sign(parameters.spin);

      strcpy((*system)->clusters[1].name, parameters.name);

      (*system)->q = q;
      (*system)->mu = mu;
      (*system)->msig2 = msig2;
      (*system)->mrsig = mrsig;
      (*system)->vrel = vrel;
      (*system)->b = b;
      (*system)->dx = dx;
      (*system)->eta = parameters.eta;
      (*system)->tenc = tenc;
      (*system)->Ehat = Ehat;
      (*system)->Lhat = Lhat;

      (*system)->lambda_orb = lambda_orb;
      
      (*system)->clusters[0].vrms = sig1;
      (*system)->clusters[1].vrms = sig2;
      
      (*system)->clusters[0].M = m1;
      (*system)->clusters[1].M = m2;
      
      (*system)->clusters[0].rvir = r1;
      (*system)->clusters[1].rvir = r2;
    }
}

/*************************************************/
void create(SYSTEM **system)
{     
  CLUSTER *cluster = NULL;
  // Loop over all clusters and assign masses, positions and velocities
  for (int i=0; i<(*system)->Ncl; i++){
    cluster = &(*system)->clusters[i];
    set_scalings(cluster);
    imf(cluster);
    get_pos_vel(cluster);

    scale(cluster); 

  }

  // Add orbital motion and compute total angular momentum
  if ((*system)->Ncl == 2){
    twobody_orbit(*system);
    scale_system(*system);
    (*system)->Lz = Lz(*system); 
    (*system)->lambda = (*system)->Lz*sqrt(0.25);
  }
  // Set physical scaling
  (*system)->mstar = (*system)->N * (*system)->clusters[0].mmean;
  (*system)->tstar = sqrt(cube((*system)->rstar)/(G*(*system)->mstar));
  (*system)->vstar = pcMyr2kms*(*system)->rstar/(*system)->tstar;

}

/*************************************************/
void set_scalings(CLUSTER *cluster)
{  
  switch (cluster->model)
    {
    case (0): 
      // Cored gamma/eta (Dehnen 1993; Tremaine et al. 1994)
      cluster->rh_over_r0 = 3.8473;
      cluster->rv_over_r0 = 5.0;
      break;
    case (1):
      // Hernquist (1990)
      cluster->rh_over_r0 = 1.0 + sqrt(2.0);
      cluster->rv_over_r0 = 3.0;
      break;
    case (2):
      // Jaffe (1983)
      cluster->rh_over_r0 = 1.0;
      cluster->rv_over_r0 = 1.0;
      break;
    case (3):
      // Henon (1959) "Isochrone" 
      cluster->rh_over_r0 = 3.0603;
      cluster->rv_over_r0 = 6./(3*PI-8);      
      break;
    case (4):
      // Plummer (1911)
      cluster->rh_over_r0 = 1.0/sqrt(pow(2.0,2.0/3.0)-1.0);
      cluster->rv_over_r0 = 16.0/(3.0*PI);
      break;
    case (5):
      // gamma = 1.5 Dehnen (1993)
      cluster->rh_over_r0 = 1.0/(pow(2.0,2.0/3.0)-1.0);
      cluster->rv_over_r0 = 2.0;
      break;
}
  cluster->rh_over_rv = cluster->rh_over_r0/cluster->rv_over_r0;
  cluster->rmax_over_r0 = cluster->rcut*cluster->rh_over_r0;

  // Calculate cluster relaxation time 
  double gamma = 0.11;
  switch (cluster->imftype)
    {
    case (0):
      // Giersz & Heggie 1994, MNRAS, 268, 257 
      gamma = 0.11;
      break;
    case (1):
      // Giersz & Heggie 1996, MNRAS, 279, 1037
      gamma = 0.02;
      break;
    }
  cluster->trh = 0.138*cluster->N*sqrt(cube(cluster->rvir*cluster->rh_over_rv))/log(gamma*cluster->N);

}


/*************************************************/
void imf(CLUSTER *cluster)
{
  int i, nrem;
  double zm, m, mrem, mtot;
  double c[12], mass[cluster->N];
  double mup = cluster->mup; // fixed for the moment
  
  switch (cluster->imftype)
    {
    case (0):
      // equal masses
      cluster->mmean = 1.0;
      for (i=0; i<cluster->N; i++){
	cluster->stars[i].mass = cluster->M/(double)cluster->N;
      }
      break;

    case (1):
      // Kroupa (2001) between 0.1 Msun and mup
      // The masses are sampled randomly, but the total mass is 
      // forced to be the analytical expected value
      c[0] = pow(0.1,-0.3);
      c[1] = pow(0.5,-0.3);
      c[2] = pow(0.5,-1.3);
      c[3] = pow(mup,-1.3);
      c[4] = pow(0.5,0.7);
      c[5] = pow(0.1,0.7);
      c[6] = pow(mup,-0.3);

      c[7] = (c[0] - c[1])/0.3;
      c[8] = (c[2] - c[3])/1.3;

      c[9] = (c[4] - c[5])/0.7;
      c[10] = (c[1] - c[6])/0.3;
      c[11] = 2.0*c[7] + c[8];
      
      cluster->mmean = (c[9] + 0.5*c[10])/(c[7] + 0.5*c[8]);
      mtot = cluster->mmean * cluster->N;
      
      m=0.0;
      for (i=0; i<cluster->N-1; ++i)
	{
	  int cont = 1.0;
	  while (cont){
	    double xx = myrand();
	    
	    if (xx >= 2.0*c[7]/c[11]){
	      zm = pow(2.6*c[7] + c[2] - xx*1.3*c[11],-1.0/1.3);
	    }
	    else{
	      zm = pow(0.15*c[11]*xx + c[1],-1.0/0.3);
	    }
	    nrem = cluster->N - i - 1;
	    mrem = mtot - (m+zm);
	    if ((mrem>nrem*0.2)&&(mrem<nrem*mup/3.0))
	      cont=0;
	  }
	  mass[i] = zm;
	  m += zm;
	}
      // Assign remaining mass to last star to get exactly M=mN
      mass[i] = mtot - m;
      m = mtot;
      // Sort and assign
      shell(mass,cluster->N);

      for (i=0; i<cluster->N; i++){
	cluster->stars[i].mass=mass[i]*cluster->M/mtot;
      }      
    }
}

/*************************************************/
void single_pos_vel(double pos[3], double vel[3], double rmax_over_r0, double ra, int model)
{
  double r, r2, v, eta, v2, a[5];
  double vr, vt,  vtheta, vphi, theta, phi;

  for (int j=0;j<5;j++)
    a[j]=myrand();
      
  // Position
  r = get_r(rmax_over_r0, model);
  r2 = sqr(r);

  pos[0] = (1.0 - 2.0*a[0])*r;
  pos[1] = sqrt(r2 - sqr(pos[0]))*cos(TWOPI*a[1]);
  pos[2] = sqrt(r2 - sqr(pos[0]))*sin(TWOPI*a[1]);
      
  // Velocity
  if (ra >= 999){
    // Isotropic 
    v = get_v(r, model);
    v2 = sqr(v);
	
    vel[0] = (1.0 - 2.0*a[2])*v;
    vel[1] = sqrt(v2 - sqr(vel[0]))*cos(TWOPI*a[3]);
    vel[2] = sqrt(v2 - sqr(vel[0]))*sin(TWOPI*a[3]);
  }
  else{
    // Anisotropic Osipkov-Merritt model 
    get_osipkov_merrit_v_eta(r, ra, model, &v, &eta);
    v2 = sqr(v);

    // Create spherical velocity coordinates:
    vt = v*sin(eta);
    vphi = vt*cos(TWOPI*a[4]);
    vtheta = vt*sin(TWOPI*a[4]);
    vr = v*cos(eta);

    // Convert to Cartesian velocity components
    theta = acos(pos[2]/r);
    phi = atan2(pos[1],pos[0]);
    
    vel[0] = vr*sin(theta)*cos(phi) + vtheta*cos(theta)*cos(phi) - vphi*sin(phi);
    vel[1] = vr*sin(theta)*sin(phi) + vtheta*cos(theta)*sin(phi) + vphi*cos(phi);
    vel[2] = vr*cos(theta) - vtheta*sin(theta);
  }
}
/*************************************************/
void get_analytic_phi(int model, double pos[3], double *phi)
{
  double p, r, r2; 

  r2 = 0.0;
  for (int k=0; k<3; k++)
    r2 += sqr(pos[k]);

  r = sqrt(r2);

  switch (model)
    {
      // TBD: carefully check 
    case (0): 
      // Cored gamma=0 model (Dehnen 1993)
      *phi = -0.5*(1.0+2.0*r)/sqr(1.0+r);
      break;
    case (1): 
      // Hernquist (1990) model)
      *phi = -1.0/(1.0+r);
      break;
    case (2): 
      // Jaffe (1993) model)
      *phi = -log(1.0+1.0/r);
      break;
    case (3): 
      // Henon (1959) "Isochrone" 
      p = sqrt(1.0+r2);
      *phi = -1.0/(1.0+p);
      break;
    case (4): 
      // Plummer (1911) 
      *phi = -1.0/sqrt(1.0+r2);
      break;
    case (5): 
      // gamma = 1.5 (Dehnen 1993) 
      *phi = -2.0*(1 - sqrt(r/(1.0+r)) );
      break;

    }
}


/*************************************************/
void get_pos_vel(CLUSTER *cluster)
{
  // Sample positions and velocities. Total mass assumed to be 1 at this stage.
  double pos[3], vel[3], lz, R2, v2;
  int idmin, idmax;
  cluster->Lz = 0.0;
  cluster->Krot = 0.0;


  for (int i=0; i<cluster->N; i++)
    {
      single_pos_vel(pos, vel, cluster->rmax_over_r0, cluster->ra, cluster->model);

      for (int k=0; k<3; k++)
	{
	  cluster->stars[i].pos[k] = pos[k];
	  cluster->stars[i].vel[k] = vel[k];
	}
    }

  // Add mass segregation following Baumgardt et al. 2008 (Appendix) recipe 
  // !! Implemented Jan 20 2017, some additional testing would be good !!
  if (cluster->fsegr>0)
    {
      int Np = 20*cluster->N;
      STAR *temp_stars = calloc(Np, sizeof(STAR)); // TBD: Can be done at start?

      double *cmass = malloc((cluster->N+1)*sizeof(double));

      // Generate  more particles to sample from
      for (int i=0; i<Np; i++)
	{
	  single_pos_vel(temp_stars[i].pos, temp_stars[i].vel, cluster->rmax_over_r0, \
			 cluster->ra, cluster->model);
	  get_analytic_phi(cluster->model, temp_stars[i].pos, &temp_stars[i].phi);
	  temp_stars[i].E = temp_stars[i].phi + 0.5*vec_sqr(temp_stars[i].vel);
	}
      
      // Sort stars array by energy
      shell_stars_E(temp_stars, Np); 

      // Make cumulative mass array
      // Note division by cluster->M to give range between 0 and 1
      cmass[0] = 0.0; 
      cmass[1] = cluster->stars[0].mass/cluster->M;
      for (int i=1; i<cluster->N; i++)
	cmass[i+1] = cmass[i]+ cluster->stars[i].mass/cluster->M;

      // Pick particles based on energy
      for (int i=0; i<cluster->N; i++)
	{
	  if (myrand()<cluster->fsegr)
	    {
	      idmin = Np*cmass[i];
	      idmax = Np*cmass[i+1];
	      int ii = myrandint(idmin, idmax);
	      temp_stars[ii].mass = cluster->stars[i].mass;
	      cluster->stars[i] = temp_stars[ii];
	    }
	}

      free(temp_stars);
      free(cmass);
    }


  // Continue ...
  for (int i=0; i<cluster->N; i++)
    {
      for (int k=0; k<3; k++)
	cluster->compos[k] += cluster->stars[i].mass * cluster->stars[i].pos[k];

      v2 = 0.0;
      for (int k=0; k<3; k++)
	{
	  v2 += sqr(cluster->stars[i].vel[k]);
	  cluster->comvel[k] += cluster->stars[i].mass * cluster->stars[i].vel[k];
	}

      cluster->stars[i].kin = 0.5*cluster->stars[i].mass*v2;      

      // Add angular momentum
      lz = cluster->stars[i].pos[0]*cluster->stars[i].vel[1] - 
	cluster->stars[i].pos[1]*cluster->stars[i].vel[0];
      if (cluster->spin!=0){
	if (cluster->spin!=sign(lz)){
	  if (myrand()<sqrt(cluster->frot)){
	    cluster->stars[i].vel[0] *= -1.0;
	    cluster->stars[i].vel[1] *= -1.0;
	    lz *= -1.0;
	  }
	}
      }
      cluster->Lz += cluster->stars[i].mass*lz;

      // Add mass weighted mean v_phi to Krot
      R2 = pow(cluster->stars[i].pos[0],2.0) + pow(cluster->stars[i].pos[1],2.0);
      cluster->Krot += cluster->stars[i].mass*lz/sqrt(R2);
    }  

  // Krot = 0.5*M*<v_phi>^2
  cluster->Krot = 0.5*pow(cluster->Krot,2.0);  
}



/*************************************************/
double get_r(double rmax_over_r0, int model)
{
  double r, a, a2, p1, p2;
  
  r=2.0*rmax_over_r0;
  while (r > rmax_over_r0)
    {
      a=myrand();    
      switch (model)
	{
	case(0):
	  // Cored gamma=0 model (Dehnen 1993)
	  p1 = pow(a,1.0/3.0);
	  r = p1/(1.0-p1);
	  break;
	case (1):
	  // Hernquist (1990) model
	  r = (a+sqrt(a))/(1.0-a);
	  break;
	case (2):
	  // Jaffe (1993) model
	  r = a/(1.0-a);
	  break;
	case (3):
	  // "Isochrone" model (Henon 1959)
	  a2 = sqr(a);	
	  p2 = pow(2.0*a*(27.0 - 1.5*sqrt(81.0-3.0*a2)*(a2*a2+a2-2.0) + a2*(18.0*a2-13.0)),1.0/3.0);
	  r = (4.0*a - a2*(3.0*a2 - 19.0)/p2 + p2)/(3.0*(1.0-a2));
	  break;
	case (4):
	  // Plummer (1911) model
	  r = 1.0/sqrt(pow(a,-2.0/3.0) - 1.0);		
	  break;
	case (5):
	  // gamma = 1.5 Dehnen 1993
	  r = 1.0/(pow(a,-2.0/3.0) - 1.0);		
	  break;
	}
    }
  return r;
}

/*************************************************/
double get_v(double r, int model)
{
  // Sample velocity from distribution function (DF)
  // Note: energy is defined to be positive: E = -0.5v2 - phi = 0.5*(vesc2 -v2)

  double v, v2, q, q2, E, E2, E3, PE, f1, f2;
    
  double r2 = sqr(r);
  double f = 1.0 + r;
  double g = 0.0; 
  double gmax = 1.0;
  double vesc2 = 0.0;
  double df = 0.0;

  while (gmax > g){
    q = myrand();  // q = v/v_esc
    q2 = sqr(q);
    switch (model)
      {
      case(0):
	// Cored gamma=0 (Dehnen 1993)
	// DF from equation (21) in Tremaine et al. (1994)
	vesc2 = 1.0-r2/sqr(f);
	E = 0.5*vesc2*(1.0-q2);  	
	PE = sqrt(2.0*E);            
	df = 2.0*PE*(3.0-4.0*E)/(1.0-2.0*E) + 3.0*log((1.0-PE)/(1.0+PE));     
	gmax = 2.1*pow(1+1.1*r,-3.5);
	break;
      case (1):
	// Hernquist, ApJ, 356, 359 (1990)
	vesc2 = 2.0/(1.0+r);
	E = 0.5*vesc2*(1.0-q2);  
	E2 = sqr(E);
	df = 3.0*asin(sqrt(E)) + sqrt(E*(1.0-E)) * (1.0-2.0*E) * (8.0*E2-8.0*E-3.0);
	df *= pow(1.0-E,-2.5);
	gmax = 2.5*pow(r,-1.5)*pow(1+0.3*r2,-1.0);
	break;
      case (2):
	// Jaffe (1983)
	vesc2 = 2.0*log(f/r);
	E = 0.5*vesc2*(1.0-q2);  
	f1 = 0.5*sqrt(PI)*exp(2.0*E)*erf(sqrt(2.0*E)) + dawson(sqrt(2.0*E));
	f2 = 0.5*sqrt(PI)*exp(E)*erf(sqrt(E)) + dawson(sqrt(E));
	df = f1 - sqrt(2.0)*f2;
	gmax = 0.4*pow(r,-2.0)*pow(1+0.75*r,-1.5);
	break; 
      case (3):
	// Henon (1959) "Isochrone" sphere
	// DF from equation (4.54) in Binney & Tremaine 2nd edition (2007)
	vesc2 = 2.0/(1.0+sqrt(1.0+r2));  
	E = 0.5*vesc2*(1.0-q2); 
	E2 = sqr(E);      
	df = 27.0 - 66.0*E + 320.0*E2 - 240.0*E2*E + 64.0*E2*E2;
	df += 3.0*(16.0*E2+28.0*E-9.0)*asin(sqrt(E))/sqrt(E*(1.0-E));
	df *= sqrt(E)/pow(1.0-E,4.0);
	gmax = 60.0*pow(1.0+0.7*r2,-1.75);
	break;
      case (4):
	// Plummer (1911)
	vesc2 = 2.0/sqrt(1.0+r2);
	E = 0.5*vesc2*(1.0 - q2);
	df = pow(E,3.5);
	gmax = 0.2*pow(1+r2,-2.25);
	break;
      case (5):
	// gamma=1.5 Dehnen (1993) eq A16
	vesc2 = 4.0*(1 - sqrt(r/f) );
	E = 0.5*vesc2*(1.0 - q2);
	E2 = E*E;
	E3 = E*E2;

	f1 = -9.0/16.0 -99.0/16.0*E + 405.0/8.0*E2 - 3705.0/56.*E3 + 561.0/14.0*E2*E2;
	f1 += -181.0/14.*E3*E2 + 15.0/7.0*E3*E3 - E3*E3*E/7.0;
	f1 += 3.0*(3.0+32.0*E-8.0*E2)/(8.0*sqrt(E*(2.0-E)))*asin(sqrt(E/2.0));
	df = sqrt(E)/pow(2.0-E,4.0)*f1;

	gmax = 0.4*pow(r,-1.75)*pow(1+0.58*pow(r,1.2),-1.75/1.2); 
	break;
      }
    v2 = vesc2*q2;
    g = v2*df;
    gmax *= myrand();
  }
  v = sqrt(v2);
  return v;
}
/*************************************************/
void get_osipkov_merrit_v_eta(double r, double ra, int model, double *v, double *eta)
{
  // Sample velocity and angle eta needed for the anisotropic 
  // Osipkov-Merritt distribution function DF(Q), where
  //      Q = E - 0.5*J2/ra2 = 0.5*(vesc2 - v2 - vt2*sin(eta)^2/ra2)
  //
  // Introduce velocity relative to escape velcoity: q = v/v_esc

  double v2, vesc2, q, qt, qt2, q2, PQ, Q, Q2, df, f1, f2;
    
  double r2 = sqr(r);
  double ra2 = sqr(ra);
  double f = 1.0 + r;
  double g = 0.0; 
  double gmax = 1.0;
  double p = r/ra;

  df = 0.0;
  vesc2 = 0.0;
  while (gmax > g){
    // Sample q-eta pair such that Q >= 0 (Q is computed below)
    get_q_eta_pair(p, &q, eta);
    qt = q*sin(*eta);
    q2 = sqr(q);
    qt2 = sqr(qt);
    switch (model)
      {
      case(0):
	// Cored gamma=0 (Dehnen 1993)
	vesc2 = 1.0-r2/sqr(f);
	Q = 0.5*vesc2*(1.0 - q2 - qt2*r2/ra2);  
	PQ = sqrt(2.0*Q);            
	df = 2.0*PQ*(3.0-4.0*Q)/(1.0-2.0*Q) + 3.0*log((1.0-PQ)/(1.0+PQ)); 
	df += 0.5*(4.0*PQ - 3.0*asinh(sqrt(2.0*Q/(1-2*Q))))/ra2;
	gmax = 2.1*pow(1+0.65*r,-3.5);
	break;
      case (1):
	// Hernquist, ApJ, 356, 359 (1990)
	vesc2 = 2.0/(1.0+r);
	Q = 0.5*vesc2*(1.0 - q2 - qt2*r2/ra2);  
	Q2 = sqr(Q);

	// Write as isotropic DF + additional term
	df = 3.0*asin(sqrt(Q)) + sqrt(Q*(1.0-Q)) * (1.0-2.0*Q) * (8.0*Q2-8.0*Q-3.0);
	df *= pow(1.0-Q,-2.5);
	df += 8.0*sqrt(Q)*(1.0-2.0*Q)/ra2;

	gmax = 2.5*pow(r,-1.5)*pow(1+0.025*r2,-1.0);
	break;
      case (2):
	// Jaffe (1983)
	// Anisotropic DF: equation (4.291) in Binney & Tremaine (2nd edition)
	vesc2 = 2.0*log(f/r);
	Q = 0.5*vesc2*(1.0 - q2 - qt2*r2/ra2);  
	f1 = 0.5*sqrt(PI)*exp(2.0*Q)*erf(sqrt(2.0*Q)) + (1.0 + 1.0/ra2)*dawson(sqrt(2.0*Q));
	f2 = 0.5*sqrt(PI)*exp(Q)*erf(sqrt(Q)) + (1.0 + 0.5/ra2)*dawson(sqrt(Q));
	df = f1 - sqrt(2.0)*f2;
	gmax = 2*0.4/sqr(r)*pow(1+0.1*r,-1.5);
	break;
      case (3):
	// Henon (1959) Isochrone sphere
	// Anisotropic DF: equation (1) in May & Binney, MNRAS, 221, 13 (1986)
	vesc2 = 2.0/(1.0+sqrt(1.0+r2));  
	Q = 0.5*vesc2*(1.0 - q2 - qt2*r2/ra2);  
	Q2 = sqr(Q);

	// Write as isotropic DF + extra term
	df = 27.0  - 66.0*Q + 320.0*Q2  -240.0*Q2*Q + 64.0*Q2*Q2;
	df += 3.0*((16.0-8.0/ra2)*Q2 + (28.0-44.0/ra2)*Q-9.0+17.0/ra2)*asin(sqrt(Q))/sqrt(Q*(1.0-Q));
	df *= sqrt(Q)/pow(1.0-Q,4.0);

	df += ((77.0+286.0*Q + 136.0*Q2)/ra2 +32.0*Q2/ra2  )*sqrt(Q)/pow(1.0-Q,4.0);
	gmax = 60.0*pow(1.0+0.15*r2,-1.75);
	break;
      case (4):
	// Plummer (1911)
	// Anisotropic DF: equation (45) in Merritt (1985)
	vesc2 = 2.0/sqrt(1.0+r2);
	Q = 0.5*vesc2*(1.0 - q2 - qt2*r2/ra2);  

	df = pow(Q,3.5)*( (1.0 - 1.0/ra2) + (63.0/144.0)*pow(Q,-2.0)/ra2);
	gmax = 0.22*pow(1+0.3*r2,-2.25);
	break;


      case (5):
	// Anisotropic DF: equation A18 Dehnen (1993)
	//	vesc2 = 2.0/sqrt(1.0+r2);
	//	Q = 0.5*vesc2*(1.0 - q2 - qt2*r2/ra2);  

	//	df = pow(Q,3.5)*( (1.0 - 1.0/ra2) + (63.0/144.0)*pow(Q,-2.0)/ra2);
	//	gmax = 0.22*pow(1+0.3*r2,-2.25);
	break;
}
    v2 = q2*vesc2;
    g = sin(*eta)*v2*df;
    gmax *= myrand();
  }
  *v = sqrt(v2);
}

/*************************************************/
void get_q_eta_pair(double p, double *q, double *eta)
{
  // Get a pair of q=v/v_esc and eta values needed for 
  // Osipkov-Merritt model, sampled such that Q > 0
  *q = 1.0;
  double qmax = 0.0;
  double a = 2.0*p/PI;
  double x = 0;

  // Sample points uniformly below the curve 1/sqrt(1+(a*x)**2)
  // Reject points above 1/sqrt(1+(p*sin(eta))**2)
  while (*q > qmax){
    *eta = sinh(myrand()*asinh(p))/a;
    x = myrand();
    *q = x/sqrt(1.0 + sqr(a*(*eta)) );
    qmax = 1.0/sqrt(1.0 + sqr(p*sin(*eta)) );
  } 
  // Assign half of the values to 0.5 < eta/pi < 1
  if ((int)(myrand()+0.5))
    *eta = PI - *eta;
}

/*************************************************/
void scale(CLUSTER *cluster)
{
  double rfac = cluster->rvir/cluster->rv_over_r0;
  double vfac = sqrt(cluster->M/rfac);
  cluster->Lz *= rfac*vfac;
  cluster->Krot *= vfac*vfac;
  if (cluster->doPhi){
    int N = (int)cluster->N;
    float *m = (float *)malloc(N*sizeof(float));
    float *x = (float *)malloc(N*sizeof(float));
    float *y = (float *)malloc(N*sizeof(float));
    float *z = (float *)malloc(N*sizeof(float));
    float *phi = (float *)malloc(N*sizeof(float));
  

    for (int i=0; i<cluster->N; i++)
      {
	for (int k=0; k<3; k++){	
 	  // Scale to COM pos = vel = 0 and Heggie & Mathieu (1986) N-body units	
	  cluster->stars[i].pos[k] -= cluster->compos[k];
	  cluster->stars[i].vel[k] -= cluster->comvel[k];
	  // Reset COM pos and vel
	  cluster->compos[k] = 0.0;
	  cluster->comvel[k] = 0.0;
	  // Scale to desired r_vir. 
	  cluster->stars[i].pos[k] *= rfac;	
	  cluster->stars[i].vel[k] *= vfac;
	}
	// copy to temp arrays for potential calculation on GPU
	m[i] = cluster->stars[i].mass;
	x[i] = cluster->stars[i].pos[0];
	y[i] = cluster->stars[i].pos[1];
	z[i] = cluster->stars[i].pos[2];
	phi[i] = 0.0; 
      }
    // This function is executed on the CPU (defined in pot.c) or GPU (defined in gpupot.cu)
    calculate_potential(m, x, y, z, phi, N, 0);

    for (int i=0; i<cluster->N; i++){
      cluster->stars[i].phi = phi[i];
      for (int k=0; k<3; k++){
	cluster->K += 0.5*cluster->stars[i].mass*sqr(cluster->stars[i].vel[k]);
      }
    cluster->W += 0.5*cluster->stars[i].mass*cluster->stars[i].phi;
    }
    
    rfac = -cluster->W/(cluster->M*sqr(cluster->vrms));
    vfac = sqrt(0.5*cluster->M*sqr(cluster->vrms)/cluster->K);
    cluster->Lz *= rfac*vfac;
    cluster->Krot *= vfac*vfac;
    
    for (int i=0; i<cluster->N; i++){
      for (int k=0; k<3; k++){
	cluster->stars[i].pos[k] *= rfac;
	cluster->stars[i].vel[k] *= vfac;
      }
    }
    
    fprintf(stderr," Scale factor cluster %1i: pos = %8.5f; vel = %8.5f \n", 
	    cluster->id,rfac,vfac);
    
    cluster->W = -cluster->M*sqr(cluster->vrms);
    cluster->K = 0.5*cluster->M*sqr(cluster->vrms);
    
    cluster->E = cluster->K + cluster->W;
    cluster->lambda = cluster->Lz*sqrt(-cluster->E)/pow(cluster->M,2.5);
    
    free(m);
    free(x);
    free(y);
    free(z);
    free(phi);
  }
  else{
    // 26 July 2021: compute phi and energy from analytic potential
    for (int i=0; i<cluster->N; i++)
      {
	get_analytic_phi(cluster->model, cluster->stars[i].pos, &cluster->stars[i].phi);
	//	fprintf(stderr," test %10.5f \n",cluster->stars[i].phi);

	cluster->stars[i].phi *= vfac*vfac;
	for (int k=0; k<3; k++){	
	  // Scale to COM pos = vel = 0 and Heggie & Mathieu (1986) N-body units	
	  cluster->stars[i].pos[k] -= cluster->compos[k];
	  cluster->stars[i].vel[k] -= cluster->comvel[k];
	  // Reset COM pos and vel
	  cluster->compos[k] = 0.0;
	  cluster->comvel[k] = 0.0;
	  // Scale to desired r_vir. 
	  cluster->stars[i].pos[k] *= rfac;	
	  cluster->stars[i].vel[k] *= vfac;
	}
      }
    for (int i=0; i<cluster->N; i++){
      for (int k=0; k<3; k++){
	cluster->K += 0.5*cluster->stars[i].mass*sqr(cluster->stars[i].vel[k]);
      }
      cluster->W += 0.5*cluster->stars[i].mass*cluster->stars[i].phi;
    }
    cluster->E = cluster->K + cluster->W;
    cluster->lambda = cluster->Lz*sqrt(-cluster->E)/pow(cluster->M,2.5);
  }
}

/*************************************************/
void twobody_orbit(SYSTEM *system)
{
  double f1 = system->q/(1.0+system->q);
  double f2 = 1.0 - f1;
      
  system->clusters[0].compos[0] = -f1*system->dx;
  system->clusters[0].compos[1] = -f1*system->b;

  system->clusters[1].compos[0] = f2*system->dx;
  system->clusters[1].compos[1] = f2*system->b;

  system->clusters[0].comvel[0] = f1*system->vrel;
  system->clusters[1].comvel[0] = -f2*system->vrel;

  // Check whether clusters overlap
  double size = system->clusters[0].rcut*system->clusters[0].rh_over_rv*(system->clusters[0].rvir + system->clusters[1].rvir);
  if (system->d <= size){
    fprintf(stderr," *** \n");
    fprintf(stderr," *** Warning: clusters overlap: d = %5.1f and %3.1f(rh1+rh2) = %5.1f  \n",system->d,system->clusters[0].rcut,size);
    fprintf(stderr," *** \n");
  }
}
/*************************************************/
void scale_system(SYSTEM *system)
{ 
  // Computes the final scaling factors for pos and vel. They are applied in output()
  int N = (int)system->N;
  float *m = (float *)malloc(N*sizeof(float));
  float *x = (float *)malloc(N*sizeof(float));
  float *y = (float *)malloc(N*sizeof(float));
  float *z = (float *)malloc(N*sizeof(float));
  float *phi = (float *)malloc(N*sizeof(float));

  int N1 = system->clusters[0].N;
  double v2;
  double W = 0;
  double K = 0;
  double K_final, W_final;

  for (int i=0; i<system->Ncl; i++){
    W += system->clusters[i].W;
    for (int j=0; j<system->clusters[i].N; j++){
      m[i*N1+j] = system->clusters[i].stars[j].mass;
      x[i*N1+j] = system->clusters[i].stars[j].pos[0] + system->clusters[i].compos[0];
      y[i*N1+j] = system->clusters[i].stars[j].pos[1] + system->clusters[i].compos[1];
      z[i*N1+j] = system->clusters[i].stars[j].pos[2] + system->clusters[i].compos[2];
      phi[i*N1+j] = 0.0;
      v2 = 0;
      for (int k=0; k<3; k++){
	v2 += sqr(system->clusters[i].stars[j].vel[k] + system->clusters[i].comvel[k]);
      }      
      K += 0.5*system->clusters[i].stars[j].mass*v2;
    }
  } 
  W_final = W - system->mu/system->d;
  K_final = system->clusters[0].K + system->clusters[1].K + 0.5*system->mu*sqr(system->vrel);

  // This function is executed on the CPU (defined in pot.c) or GPU (defined in gpupot.cu)
  calculate_potential(m,x,y,z,phi,N,N1);

  for (int i=0; i < N1; i++){
    // On GPU the specific potential of the 2nd cluster is not computed, that is why the
    // binding energy of cluster pair is computed as sum of specific potentials of cluster 1
    // (without the usual /2)
    W += m[i]*phi[i];
  }

  system->rfac = W/W_final;
  system->vfac = sqrt(K_final/K);
  fprintf(stderr," Scale factor system:    pos = %8.5f; vel = %8.5f \n",system->rfac,system->vfac);

  free(m);
  free(x);
  free(y);
  free(z);
  free(phi);
}

/*************************************************/
double Lz(SYSTEM *system)
{
  //Calculate total z angular momentum
  CLUSTER *cluster = NULL;
  double Lz_tot = 0.0;
  
  for (int i=0; i<system->Ncl; i++)
    {
      cluster = &system->clusters[i];
      for (int j=0; j<cluster->N; j++)
	{
	  Lz_tot += system->rfac*system->vfac*cluster->stars[j].mass*
	    ((cluster->stars[j].pos[0] + cluster->compos[0])*(cluster->stars[j].vel[1] + cluster->comvel[1]) -
	     (cluster->stars[j].pos[1] + cluster->compos[1])*(cluster->stars[j].vel[0] + cluster->comvel[0]));
	}
    }
  return Lz_tot;
}

/*************************************************/
double dawson(double x) 
{
  // Computes the Dawson integral
  int i,n0;
  double H = 0.4;
  double A1 = 2.0/3.0;
  double A2 = 0.4;
  double A3 = (2.0/7.0);
  double d1,d2,e1,e2,sum,x2,xp,xx,ans;
  static double c[7];
  static int init = 0; //Flag is 0 if we need to initialize, else 1.
  if (init == 0) { 
    init=1;
    for (i=1;i<=6;i++) c[i]=exp(-sqr((2.0*i-1.0)*H)); 
  }
  if (fabs(x) < 0.2) { 
    //Use series expansion. 
    x2=x*x;
    ans=x*(1.0-A1*x2*(1.0-A2*x2*(1.0-A3*x2)));
  } else { 
    //Use sampling theorem representation.
    xx=fabs(x);
    n0=2*(int)(0.5*xx/H+0.5);
    xp=xx-n0*H;
    e1=exp(2.0*xp*H);
    e2=e1*e1;
    d1=n0+1;
    d2=d1-2.0;
    sum=0.0;
    for (i=1;i<=6;i++,d1+=2.0,d2-=2.0,e1*=e2)
      sum += c[i]*(e1/d1+1.0/(d2*e1)); 
    ans=0.5641895835*SIGN(exp(-xp*xp),x)*sum;
  }
  return ans; 
}
/*************************************************/
double vec_sqr(double x[3])
{
  double square = 0.0;

  for (int k=0; k<3; k++)
    square += x[k]*x[k];
      
  return square;
}


/*************************************************/
void shell(double a[], int n)
{
  int i,j,inc; 
  double v;
  inc = 1;

  do {
    inc *= 3;
    inc++;
  } while (inc <= n); 
  do {
    inc /= 3;
    for (i=inc;i<n;i++) {
      v=a[i];
      j=i;
      while (a[j-inc] > v) {
	a[j]=a[j-inc];
	j -= inc;
	if (j < inc) break;
      }
      a[j]=v; 
    }
  } while (inc >= 1);
  
  // Reverse
  for (i=0; i<(int)n/2; i++){
    v=a[n-i-1];
    a[n-i-1]=a[i];
    a[i]=v;
  } 
}

/*************************************************/
void shell_stars_E(STAR *stars, int n)
{
  // Sort array of stars in energy, most negative first
  int i,j,inc; 
  double v;
  STAR vstar;
  inc = 1;
  
  do {
    inc *= 3;
    inc++;
  } while (inc <= n); 
  do {
    inc /= 3;
    for (i=inc;i<n;i++) {
      v=stars[i].E;
      vstar = stars[i];
      j=i;
      while (stars[j-inc].E > v) {
	stars[j] =stars[j-inc];

	j -= inc;
	if (j < inc) break;
      }
      stars[j]=vstar; 
    }
  } while (inc >= 1);
  
}

/*************************************************/
void output(SYSTEM *system)
{
  CLUSTER *cluster;
  fprintf(stderr," N clusters = %7i \n",system->Ncl);
  fprintf(stderr," N stars    = %7i \n",system->N);
  
  if (system->clusters[0].fsegr>0)
    fprintf(stderr," Mass segregation following Baumgardt et al. 2008, ApJ, 685, 247\n");

  fprintf(stderr,"\n CLUSTER PROPERTIES: \n");
  for (int i=0; i<system->Ncl; i++)
    { 
      cluster = &system->clusters[i];  
      double r_h = cluster->rvir*cluster->rh_over_rv;
      double rho_h = 3.0*cluster->M/(8.0*PI*cube(r_h));

      fprintf(stderr," #%i: %7s: rv/r0=%5.3f; rh/r0=%5.3f; rh/rv=%5.3f; rcut/rh=%3.1f; ra/r0=%3.1f; f_seg=%3.2f\n",i+1, 
	      cluster->name,cluster->rv_over_r0,cluster->rh_over_r0,cluster->rh_over_rv,
	      cluster->rcut,cluster->ra,cluster->fsegr);
	      
      fprintf(stderr,"     N          = %11i \n",cluster->N); 
      fprintf(stderr,"     M          = %11.3f / %11.3f Msun\n",cluster->M,cluster->M * system->mstar);
      if (cluster->imftype==1)
	fprintf(stderr,"     m_up       = %11.3f Msun \n",cluster->mup);
      fprintf(stderr,"     r_vir      = %11.3f / %11.3f pc \n",cluster->rvir, cluster->rvir * system->rstar);
      fprintf(stderr,"     r_h        = %11.3f / %11.3f pc \n",r_h,r_h*system->rstar);
      fprintf(stderr,"     rho_h      = %11.3f / %11.3f Msun/pc3 \n",rho_h, rho_h*system->mstar/cube(system->rstar));
      fprintf(stderr,"     vrms       = %11.3f / %11.3f km s-1\n",cluster->vrms, cluster->vrms * system->vstar);
      fprintf(stderr,"     vrms1D     = %11.3f / %11.3f km s-1\n",cluster->vrms/sqrt(3.0), 
	      cluster->vrms * system->vstar/sqrt(3.0));
      fprintf(stderr,"     trh        = %11.3f / %11.3f Myr\n",cluster->trh, cluster->trh * system->tstar);
      fprintf(stderr,"     E          = %11.3f \n",cluster->E); 
      fprintf(stderr,"     W          = %11.3f \n",cluster->W); 
      fprintf(stderr,"     Lz / spin  = %11.3f / %11i \n",cluster->Lz, cluster->spin);
      fprintf(stderr,"     Krot/K     = %11.3f \n",cluster->Krot/cluster->K);
      fprintf(stderr,"     lambda     = %11.3f \n",cluster->lambda);
    }

  if (system->Ncl == 2)
    {
    fprintf(stderr,"\n SYSTEM PROPERTIES: \n");
    fprintf(stderr,"   e_hat      = %11.3f \n",system->Ehat);
    fprintf(stderr,"   l_hat      = %11.3f \n",system->Lhat);
    fprintf(stderr,"   d          = %11.3f \n",system->d);
    fprintf(stderr,"   q          = %11.3f \n",system->q);
    fprintf(stderr,"   eta        = %11.3f \n",system->eta);
    fprintf(stderr,"   mu         = %11.3f \n",system->mu);
    fprintf(stderr,"   E_orb      = %11.3f \n",system->mu*system->Ehat*0.5*system->msig2);
    fprintf(stderr,"   L_orb      = %11.3f \n",system->mu*system->Lhat*system->mrsig);
    fprintf(stderr,"   lambda_orb = %11.3f \n",system->lambda_orb);
    fprintf(stderr,"   seed       = %11i \n",system->seed);

    fprintf(stderr,"\n");	
    fprintf(stderr,"   v_rel      = %11.3f / %11.3f km s-1\n",
	    system->vrel,system->vrel * system->vstar); 
    fprintf(stderr,"   b          = %11.3f / %11.3f pc\n",
	    system->b,system->b * system->rstar);
    fprintf(stderr,"   tenc       = %11.3f / %11.3f Myr \n", 
	    system->tenc,  system->tenc*system->tstar);
    fprintf(stderr,"   vrms       = %11.3f / %11.3f km s-1\n",
	    sqrt(system->msig2), sqrt(system->msig2) * system->vstar);
    fprintf(stderr,"   vrms1D     = %11.3f / %11.3f km s-1 \n",
	    sqrt(system->msig2/3.0), sqrt(system->msig2/3.0) * system->vstar);
    fprintf(stderr,"   Lz         = %11.3f \n",system->Lz);
    fprintf(stderr,"   lambda     = %11.3f \n",system->lambda);
    }
  
  
  for (int i=0; i<system->Ncl; i++)
    {
      cluster = &system->clusters[i];  
      for (int j=0;j<cluster->N;j++){
	if (cluster->print_phi)
	  {
	    printf("%18.10e   %18.10e %18.10e %18.10e   %18.10e %18.10e %18.10e  %18.10e\n",
		   cluster->stars[j].mass,
		   (cluster->stars[j].pos[0] + cluster->compos[0])*system->rfac,
		   (cluster->stars[j].pos[1] + cluster->compos[1])*system->rfac,
		   (cluster->stars[j].pos[2] + cluster->compos[2])*system->rfac,
		   (cluster->stars[j].vel[0] + cluster->comvel[0])*system->vfac,
		   (cluster->stars[j].vel[1] + cluster->comvel[1])*system->vfac,
		   (cluster->stars[j].vel[2] + cluster->comvel[2])*system->vfac,
		   (cluster->stars[j].phi*pow(system->vfac,2.0))); 
	  }else
	  {
	    printf("%18.10e   %18.10e %18.10e %18.10e   %18.10e %18.10e %18.10e  \n",
		   cluster->stars[j].mass,
		   (cluster->stars[j].pos[0] + cluster->compos[0])*system->rfac,
		   (cluster->stars[j].pos[1] + cluster->compos[1])*system->rfac,
		   (cluster->stars[j].pos[2] + cluster->compos[2])*system->rfac,
		   (cluster->stars[j].vel[0] + cluster->comvel[0])*system->vfac,
		   (cluster->stars[j].vel[1] + cluster->comvel[1])*system->vfac,
		   (cluster->stars[j].vel[2] + cluster->comvel[2])*system->vfac);
	  }
      }
    }  

  // Save individual clusters in case of multiple clusters
  if (system->Ncl > 1)
    {
      FILE *p = NULL;
      char file[10];
      
      for (int i=0; i<system->Ncl; i++)
	{
	  sprintf(file,"cluster.%d",i+1);
	  p = fopen(file,"w");
	  
	  cluster = &system->clusters[i];  
	  for (int j=0;j<cluster->N;j++){
	    fprintf(p, "%18.10e   %18.10e %18.10e %18.10e   %18.10e %18.10e %18.10e \n",
		    cluster->stars[j].mass,
		    cluster->stars[j].pos[0],
		    cluster->stars[j].pos[1],
		    cluster->stars[j].pos[2],
		    cluster->stars[j].vel[0],
		    cluster->stars[j].vel[1],
		    cluster->stars[j].vel[2]); 
	  }
	}
    }
      
}

/*************************************************/
void free_memory(SYSTEM *system)
{
  for (int i = 0; i < system->Ncl; ++i)
    free(system->clusters[i].stars);
  free(system);
}

/*************************************************/
int main(int argc, char** argv)
{
  SYSTEM *system = NULL;
  INPUT parameters;

  get_args(argc, argv, &parameters);   
  allocate_memory(&system, &parameters);
  initialize(&system, parameters);
  create(&system);
  output(system);
  free_memory(system);
}
