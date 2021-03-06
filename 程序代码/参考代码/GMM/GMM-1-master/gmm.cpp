// This program does K-mean clustering and EM iterations
// input data data.txt contains 1000 2D samples, generated by 5 mixture gmm

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <vector>
 
using namespace std;

// Global variable settings:
const int DIM=2;        // This is data dimensionality
const int Num_Mix=10;     // Number of GMM mixtures
const int NumData=500;   // This is the number of data to be used
const int Min_samples=2;  // This is the minimum number of samples in each cluster
const float Min_weight=0.02; // This is the weight floor for k-mean
const int Max_Iter=1000; // This is the maximum number of iterations for k-mean
const float threshold=0.0001; // This is the stop criteria for k-mean
const float Infinity =1e+32;
const float Perturb=0.001; // When a bad cell appears, replace its center by the center
                          // of the most popular cell plus this perturbation
const float threshold_EM=0.0001; // stop criteria for EM

// This structure is for one mixture
struct Mix 
{
   float Mean[DIM];  // mean vector
   float variance[DIM]; // variance vector
   float weight;        // weight
   vector <int> Sample_Index; // This vector stores the indices of data points which belongs to this mixture  
   int Total_Sample;   // This counts the total number of samples of this mixture
};

class GMM   // This class creates a GMM
{
 public:
   GMM() {}  // Constructor
   void Read_Feature(char *);  // Read in features from a file
   void Random_Init();                 // Randomly assign initial centers
   void Iterate();                     // Do k-mean iterations
   void EM();                          // Do EM

 private:
   float Feature[NumData][DIM];  // Each row is a feature
   Mix Mixtures[Num_Mix];        // Each element is a mixture
   float E_Prob[Num_Mix][NumData]; // This matrix stores Pr(class i|data j) in E step
   void Recluster();             // Recluster samples in k-mean
   int Check_Badcell();        // Check if there is a mixture with less than Min_samples in k-mean 
   void Adjust_Badmean(int);   // If there is a bad cell, add a new center in the most popular cluster
   float  Update_Mean();           // Update centers after reclustering
   void Update_variance();         // update variances after k-mean
   void Update_weight();           // update weights after k-mean
   float Distance(int, int);     // Compute L2-norm
   int find_closest_mixture(int);  // find closest mixture to a pattern in k-mean
   float Compute_Gaussian(int,float *); // Compute gaussian for a mixture
   void E_step();                       // This performs E-step in EM
   float M_step();                       // This performs M-step in EM
};
// ---------------------------------------------------End  Class Definition ---------------------------------------------------- 

int main()
{
  char filename[]="data.txt";
  GMM gmm;     // construct a gmm
  gmm.Read_Feature(filename);
  gmm.Random_Init();
  gmm.Iterate();
  gmm.EM();
  return 0;
}
//----------------------------------------------------- End main ---------------------------------------------------------------

// ----------------------------------------------------Class member functions --------------------------------------------------

void GMM::Read_Feature(char *filename)   // This function reads features from a file, and store in the feature matrix
{
   FILE *fin;
   int i,j;
   cout<<"Reading features"<<endl;
if((fin=fopen(filename,"r"))==NULL)      // file error
   { printf("Cannot open input file %s\n",filename);
	 exit(0);
   }

for (i=0; i<NumData; i++)    // Reading data
{
  for (j=0;j<DIM; j++)  
   {   
      fscanf(fin,"%f", &Feature[i][j]);
   }
} 
cout<<"Reading features complete"<<endl;
fclose(fin);
}

void GMM::Random_Init() // Just use the first # of mixtures features to initialize the centers respectivly
{
  printf("Now, initialize mixture mean\n");
  for (int i=0; i<Num_Mix; i++)    
      for (int j=0; j<DIM; j++) 
         Mixtures[i].Mean[j]=Feature[i][j]; // Initialize mean
  
  for (int i=0; i<Num_Mix; i++)
     {printf("Initial mean of mixture %d is: ",i);
      for (int j=0; j<DIM; j++)
	     printf("%f ", Mixtures[i].Mean[j]);
	  printf ("\n");
	 }
  printf("Mean initialization complete\n");
}


void GMM::Recluster()    // This function recluster samples according to current means
{
 int i,closest_mix;

for (i=0; i<Num_Mix;i++)
 {
   Mixtures[i].Sample_Index.clear(); //Clear sample set for each cluster first
   Mixtures[i].Total_Sample=0;
 }
for (i=0; i<NumData; i++) 
 {
   closest_mix= find_closest_mixture(i);  //Find nearest mixture for each data point
   Mixtures[closest_mix].Sample_Index.push_back(i); // cluster this data into the nearest mixture
   Mixtures[closest_mix].Total_Sample++;
 }
 for (i=0; i<Num_Mix; i++)
   printf("Current total number of samples in mixture %d is %d\n",i,Mixtures[i].Total_Sample);

}

int GMM::Check_Badcell()    // Look for a bad cell after reclustering
{ 
  int i;  
  for (i=0; i<Num_Mix; i++)
    if (Mixtures[i].Total_Sample < Min_samples)   // bad cell found
	  {
	   printf("Mixture %d is a bad cell in current iteration, fixing it\n",i);
	   return (i); // return bad cell id
	  }
  return (-1);     // otherwise, return -1 (not found)	   
}

void GMM::Adjust_Badmean(int bad_id)   // This function adjust the center of the bad cell
{
  int popular_id=0;    // This is the cluster id with the most samples
  int feat_id;
  int i,j,k;
  float New_mean[DIM];     // Use a sample from the most popular cluster to replace the bad mean
  for (i=1;i<Num_Mix; i++)    // Look for the most popular cluster
     if (Mixtures[i].Total_Sample>Mixtures[popular_id].Total_Sample)
	    popular_id=i;
  if (Mixtures[popular_id].Total_Sample<Min_samples)    // Even the most popular cluster has fewer samples than the threshold
     { cout<<"Algorithm failed"<<endl; exit (0);}
  for (k=0; k<DIM; k++)  // create a new mean for the bad cluster
      New_mean[k]=0;
  for (j=0; j<Mixtures[popular_id].Total_Sample; j++)   // compute the mean of the most popular cluster
     {feat_id=Mixtures[popular_id].Sample_Index[j];
      for (k=0; k<DIM; k++)
	    New_mean[k]=New_mean[k]+1.0/(Mixtures[popular_id].Total_Sample)*Feature[feat_id][k];
	 }

  printf("Place the center of bad mixture %d into mixture %d\n",bad_id, popular_id);   
  for (j=0; j<DIM; j++)
     Mixtures[bad_id].Mean[j]=New_mean[j]+Perturb; // use the perturbed mean of the most popular cluster
}

int GMM::find_closest_mixture(int feature)  // This function finds the closest mixture to a pattern
{
   int i, mix_id;
   float minimum_distance=Infinity, d;

 for (i=0; i<Num_Mix; i++) 
  {
     d=Distance(feature,i);
   if (d<minimum_distance) 
     {
       minimum_distance=d;
       mix_id=i;
     }
  } 
  return mix_id;
}

float GMM::Distance(int pattern, int mix)     // Compute square 2-norm between a pattern and a mixture mean
{int i;
 float d=0;
 for (i=0; i<DIM ;i++)
 {
   d+=(Mixtures[mix].Mean[i]-Feature[pattern][i])*(Mixtures[mix].Mean[i]-Feature[pattern][i]);
 } 
return d;
}

float GMM:: Update_Mean()     // This function update mean for all mixtures, and return the square change of mean over all mixtures 
{ int i,j, feat_idx,k;
  float change=0,temp;
  float new_mean[DIM];
  for (i=0;i<Num_Mix; i++) // For each mixture, update mean
    {
	  for (k=0; k<DIM; k++) new_mean[k]=0;          // clear new mean for each mixture
      for (j=0;j<Mixtures[i].Total_Sample; j++)
	    {feat_idx=Mixtures[i].Sample_Index[j];    // find the right feature
		 for (k=0;k<DIM; k++)
		   new_mean[k]=new_mean[k]+1.0F/(float)(Mixtures[i].Total_Sample)*Feature[feat_idx][k];  // compute feature mean
        }   //done with 1 mixture mean update
	  temp=0;	
	  for (k=0;k<DIM;k++)
	    {temp=temp+(new_mean[k]-Mixtures[i].Mean[k])*(new_mean[k]-Mixtures[i].Mean[k]);   // mean change in one mixture
		 Mixtures[i].Mean[k]=new_mean[k];}
	  change=change+temp;  // change is the total mean change over all clusters
    }
  for (i=0;i<Num_Mix; i++)
   { printf ("Updated mean of mixture %d is: ",i);
     for (j=0; j<DIM; j++)
	   printf("%f ",Mixtures[i].Mean[j]);
	 printf ("\n");
   }
  printf ("Total mean change=%f\n",change);   
  return change;    			    	     
}

void GMM::Update_variance()  // Compute variance of each mixture
{int i,j,k,feat_idx;
 for (i=0;i<Num_Mix; i++)
 { for (k=0; k<DIM; k++) Mixtures[i].variance[k]=0;
   for (j=0; j<Mixtures[i].Total_Sample; j++)
     {feat_idx=Mixtures[i].Sample_Index[j];
	  for (k=0;k<DIM;k++)
	   Mixtures[i].variance[k]=Mixtures[i].variance[k]+1.0F/(float)(Mixtures[i].Total_Sample)*(Feature[feat_idx][k]-Mixtures[i].Mean[k])*(Feature[feat_idx][k]-Mixtures[i].Mean[k]);
	 }
 }
 for (i=0;i<Num_Mix;i++)
 {printf("Variance of mixture %d is: ",i);
  for (j=0; j<DIM; j++)
    printf("%f ",Mixtures[i].variance[j]);
  printf("\n");
 }
}

void GMM::Update_weight()
{int i;
 int counter=0, max_id;
 float sum=1.0, max_weight=-1;
 float temp[Num_Mix];
 for (i=0; i<Num_Mix; i++)
   Mixtures[i].weight=(float)(Mixtures[i].Total_Sample)/(float) NumData; // update weight
 for (i=0; i<Num_Mix; i++)   // look for the largest weight
    if (Mixtures[i].weight>max_weight)
	{max_weight=Mixtures[i].weight;
	 max_id=i;
	}
  if (max_weight<Min_weight)
  {printf("error: max weight less than threshold\n");
   exit (0);
  }
  for (i=0;i<Num_Mix; i++)          
   {temp[i]=Mixtures[i].weight;   // temp stores weights before smoothing
	 if (Mixtures[i].weight<Min_weight)
	   {printf("Mixture %d has a bad weight, floor it to %f\n",i,Min_weight);
	    sum=sum-Mixtures[i].weight;
	    Mixtures[i].weight=Min_weight;   // floor bad weights
	    counter++;
	   }
   }
   
   if (counter>0)   // discounting un-floored weights
   {for (i=0;i<Num_Mix;i++)
      if (Mixtures[i].weight==temp[i])
	     Mixtures[i].weight=(1.0F-(float)counter*Min_weight)*(Mixtures[i].weight)/sum;
   }
   printf("Mixture weights are:\n");
   for (i=0;i<Num_Mix;i++)
     printf("%f ",Mixtures[i].weight);
   printf("\n"); 
}


void GMM::Iterate()
{ int iter=1;      // this is the counter of iterations
  int bad_cell;
  float change=Infinity;
  while ((iter<=Max_Iter)&&(change>=threshold)) 
  { printf("Now, perform iteration %d\n",iter);
	Recluster();
	bad_cell=Check_Badcell();
	if (bad_cell>=0)
	{
	 Adjust_Badmean(bad_cell);
     Recluster();
	}
	change=Update_Mean(); // Update mean after reclustering
	iter++;
  }
  Update_variance();
  Update_weight();  
} 

float GMM::Compute_Gaussian(int mix_id,float *feature) // This function computes a gaussian for a mixture
{int k;  // k is dimension
 float sum=0.0,product=1.0,output;
 float two_pi=8.0F*atan(1.0F);
 for (k=0; k<DIM; k++)
   {sum=sum+(feature[k]-Mixtures[mix_id].Mean[k])*(feature[k]-Mixtures[mix_id].Mean[k])/(Mixtures[mix_id].variance[k]);
    product=product*sqrt(Mixtures[mix_id].variance[k]);
   }
 sum=exp(-0.5*sum);
 product=1.0F/product;
 output=1.0F/pow(two_pi,(float) DIM/2.0F)*product*sum;
 return output;
}

void GMM::E_step()   // This function computes all Pr[class i|data j]
{int i,j;
 float denomenator;
 for (j=0; j<NumData; j++)
    {denomenator=0.0F;
     for (i=0; i<Num_Mix; i++)
	   {E_Prob[i][j]=Compute_Gaussian(i,Feature[j])*(Mixtures[i].weight); // Pr[class i|data j]
		denomenator=denomenator+E_Prob[i][j];
	   }
	 for (i=0; i<Num_Mix; i++)
	   E_Prob[i][j]=E_Prob[i][j]/denomenator;
	}
}

float GMM::M_step()  // This function performs the M step
{ int j,k,i;
  float sum1,sum2, sum3;
  float new_mean[DIM];
  float new_var[DIM];
  float new_weight[Num_Mix];
  float change_mean=0.0F, change_var=0.0F, change_weight=0.0F, change;
  for (j=0; j<Num_Mix; j++)
  {sum2=0.0;
   for (k=0; k<NumData; k++)
     sum2=sum2+E_Prob[j][k];
   for (i=0; i<DIM; i++)
   {sum1=0.0; sum3=0.0;
    for (k=0; k<NumData; k++)
      sum1=sum1+E_Prob[j][k]*Feature[k][i];
    new_mean[i]=sum1/sum2;	    // update mean
	change_mean=change_mean+fabs(new_mean[i]-Mixtures[j].Mean[i]); // compute change of mean
	Mixtures[j].Mean[i]=new_mean[i];
	for (k=0; k<NumData; k++)
	  sum3=sum3+E_Prob[j][k]*(Feature[k][i]-new_mean[i])*(Feature[k][i]-new_mean[i]); 
	new_var[i]=sum3/sum2;       // update variance
	change_var=change_var+fabs(new_var[i]-Mixtures[j].variance[i]); // compute change of variance
	Mixtures[j].variance[i]=new_var[i];
   }
   new_weight[j]=sum2/(float) NumData; // update weight
   change_weight=change_weight+fabs(new_weight[j]-Mixtures[j].weight); // compute change of weight
   Mixtures[j].weight=new_weight[j];
 }
 change=change_mean+change_var+change_weight;   // total change of all parameters
 return change;
}

void GMM::EM()
{float change=Infinity;
 int iter=1,i,j;
 printf("Now, perform EM iterations\n");
 while (change>=threshold_EM)
   {printf ("EM iteration %d:\n",iter);
    printf("perform E-step, compute Pr[class|data]\n");
    E_step();  // do E-step
	printf("perform M-step, update parameters: ");
    change=M_step(); // do M-step;
	printf("parameter change in iteration %d is %f\n",iter,change);
	iter++;
   } // done EM
 // Output some information
 printf("EM iterations completed\n");
 printf("Mixture weights are:\n");
 for (i=0;i<Num_Mix;i++)
   printf("%f ",Mixtures[i].weight);
 printf("\n"); 
 
 for (i=0;i<Num_Mix; i++)
 { printf ("Updated mean of mixture %d is: ",i);
   for (j=0; j<DIM; j++)
	 printf("%f ",Mixtures[i].Mean[j]);
	 printf ("\n");
 }
 
 for (i=0;i<Num_Mix;i++)
 {printf("Variance of mixture %d is: ",i);
  for (j=0; j<DIM; j++)
    printf("%f ",Mixtures[i].variance[j]);
  printf("\n");
 }  
}
