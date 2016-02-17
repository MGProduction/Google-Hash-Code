// ---------------------------------------------------------------------------
// Google Hash Code 2016 - https://hashcode.withgoogle.com/
// ---------------------------------------------------------------------------
// Online Qualification Round 
// Delivert - https://hashcodejudge.withgoogle.com/download/blob/AMIfv94iikiy5WX0_Ll6Yo5vR40DYRRv4zmjSTu_mi3ffQiqWzMDMPXoK80gUB3v9jbrbbHMsahYrO9Ppr3U20RKQDREbpCyWOoC-76_nHEfUFCDuPuERm39bSI9LmDxGplXmX_JP2fjpyeqDoKV3TgqCa58lDJyzhas13hwVwQDpFb9NsLSvSEZyUTaEP0SkDxotj2hyzdOGfSvEfZYuIwLox3ADR9T_L8WdzoNwe6gp8uvln23SYyIfhTwHh2BoK12a-G9wXDo0bcTTgH1s5lmZRHxnXPKhoJrI_ypEXvuFDwj0x3fojxlwtpimVLE_VUWtmgMw16eAbs5pelpCT5I3VC8O7UZRnV8BkdEaw9XB8T1XQdpcfLtPcn_LMI-BmstUqsmsVmj
// ---------------------------------------------------------------------------
// Team: Modena [Marco Giorgini - Andrea Capitani - Marco Bellei] #332
// The following is an enhancement of the solution we submitted during qualification round
// This code scores more than 278000 point with the three data sets
// ---------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ---------------------------------------------------------------------------

#define action_none      0
#define action_loaded    1
#define action_stop     -1

#define MAXORDERS       64
int     maxorders;

// ---------------------------------------------------------------------------

typedef struct tagPROD{
	int weight;
}PROD;
PROD*prods;

typedef struct tagWAREHOUSE{
	int  R,C;
	int  tot;
	int* prodcnt;
}WARE;
WARE*wares;

typedef struct tagORDER{
	int  id;
	int  R,C;
	int  time;
	int  tot,ltot;
	int* prodcnt;
	int* recprodcnt;
}ORDER;
ORDER*orders;

typedef struct tagDRONE{
	int  payload; 
	int  R,C;
	int  warehouseID;
	int  action;
	int  time; 
	int  orderID[MAXORDERS];
	int* prodcnt[MAXORDERS]; 
}DRONE;
DRONE*drones;

// ---------------------------------------------------------------------------

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct tagInts{
	int*Items;
	int cItems,mItems;
}Ints;

void Ints_new(Ints*i)
{
	i->cItems=0;
	i->mItems=256;
	i->Items=(int*)calloc(i->mItems,sizeof(int));
}

int Ints_push(Ints*i,int value)
{
	if(i->cItems+1>=i->mItems)
	{
		i->mItems+=256;
		i->Items=(int*)realloc(i->Items,i->mItems*sizeof(int));
	}
	i->Items[i->cItems++]=value; 
	return i->cItems-1;
}

void Ints_delete(Ints*i)
{
	free(i->Items);
	i->cItems=i->mItems=0;
}

// ---------------------------------------------------------------------------

int R,C;    // area 
int D;      // drones
int P;      // products
int W;      // warehouse
int O;      // orders
int MAXT;   // max turns
int MAXP;   // max payloads	
int SCORE;  // score
int**_cost; // cache for movement cost from warehouses to orders
Ints*dcmd;  // output buffer

// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// add commands functions (for output)
// ---------------------------------------------------------------------------

void addcmd_deliver(int droneID,int orderID,int productID,int delivercnt)
{
	Ints_push(&dcmd[droneID],droneID);
	Ints_push(&dcmd[droneID],'D');
	Ints_push(&dcmd[droneID],orderID);
	Ints_push(&dcmd[droneID],productID);
	Ints_push(&dcmd[droneID],delivercnt);
}

int addcmd_load(int droneID,int warehouseID,int productID,int add)
{
	int idx=dcmd[droneID].cItems;
	while((idx>4)&&(dcmd[droneID].Items[idx-4]=='L')&&(dcmd[droneID].Items[idx-3]==warehouseID))
	{
		if(dcmd[droneID].Items[idx-2]==productID)
		{
			dcmd[droneID].Items[idx-1]+=add;
			return 0;
		}
		idx-=5;  
	}
	Ints_push(&dcmd[droneID],droneID);
	Ints_push(&dcmd[droneID],'L');
	Ints_push(&dcmd[droneID],warehouseID);
	Ints_push(&dcmd[droneID],productID);
	Ints_push(&dcmd[droneID],add);
	return 1; 
}

// ---------------------------------------------------------------------------
// helpers 
// ---------------------------------------------------------------------------

int movecost(int R1,int C1,int R2,int C2)
{
	int   value=((R1-R2)*(R1-R2)+(C1-C2)*(C1-C2));
	double dist=ceil(sqrt((double)value));
	return (int)dist;
}

int WO_movecost(int ware,int order)
{
	if(_cost[ware][order]==0) _cost[ware][order]=movecost(wares[ware].R,wares[ware].C,orders[order].R,orders[order].C);
	return _cost[ware][order];
}

int match(ORDER*O,WARE*W,int*cnt,int*weight)
{
	int i,good=0,goodW=0,bad=0,c=0;
	if(O->ltot)
	for(i=0;i<P;i++)  
	if((W->prodcnt[i]>=O->prodcnt[i])&&O->prodcnt[i])
	{
		goodW+=O->prodcnt[i]*prods[i].weight;
		good+=O->prodcnt[i];
		c++;
	} 
	else
	bad++;
	if(cnt) *cnt=c;			
	if(weight) *weight=goodW;			
	return good;   
}

int order_compare_zero(const void*a,const void*b)
{
	ORDER*A=(ORDER*)a;
	ORDER*B=(ORDER*)b;
	int   i=0,val,costA=0x7FFFFFFF,costB=0x7FFFFFFF;
	if(val=match(A,&wares[i],NULL,NULL))
	costA=movecost(wares[i].R,wares[i].C,A->R,A->C)+val*val;
	if(val=match(B,&wares[i],NULL,NULL)) 
	costB=movecost(wares[i].R,wares[i].C,B->R,B->C)+val*val;
	return costA-costB; 
}

int wid_compare(const void*a,const void*b)
{
	int*A=(int*)a;
	int*B=(int*)b;
	return A[1]-B[1];
}

int wid_compare_rev(const void*a,const void*b)
{
	int*A=(int*)a;
	int*B=(int*)b;
	return B[1]-A[1];
}

int WD_nearestcost(int ware,int needed)
{
	int R=wares[ware].R,C=wares[ware].C,best=R*C;
	int d;
	for(d=0;(d<D)&&(best!=0);d++)
	if((drones[d].action!=action_stop)&&(drones[d].payload+needed<=MAXP))
	{
		int cost=movecost(R,C,drones[d].R,drones[d].C);
		if(cost<best)
		best=cost;
	}
	return best;   
}

int checkdronetimes()
{
	int d,good=0;
	for(d=0;d<D;d++)
	if(drones[d].time<MAXT)
	good++;
	else
	drones[d].action=action_stop;
	return good;
}

int checkordertoprocess()
{
	int o,cnt=0;
	for(o=0;o<O;o++)
	if(orders[o].tot)
	cnt++; 
	o=0;  
	return cnt;
}

int ordercomplete(DRONE*drone,int orderID)
{
	SCORE+=(int)ceil((((float)MAXT-(float)orders[orderID].time)/(float)MAXT)*100.0f);
	return SCORE;
}

int canaddordertodrone(DRONE*d,int warehouseID,int orderID,int full)
{
	int f;
	for(f=0;f<maxorders;f++)
	if(d->orderID[f]==orderID)
	return 1;
	else
	if(d->orderID[f]==-1)	
	return 1;
	return 0;  
}

// ---------------------------------------------------------------------------
// input/output (and free everything) functions
// ---------------------------------------------------------------------------

int readinput(const char*filename)
{
	FILE*f=fopen(filename,"rb");
	if(f)
	{
		int i,j;

		SCORE=0;

		fscanf(f,"%d %d %d %d %d\n",&R,&C,&D,&MAXT,&MAXP);
		// products
		fscanf(f,"%d\n",&P);
		prods=(PROD*)calloc(P,sizeof(PROD));
		for(i=0;i<P;i++)
		fscanf(f,"%d",&prods[i].weight);

		fscanf(f,"\n"); 
		// warehouse
		fscanf(f,"%d\n",&W);
		wares=(WARE*)calloc(W,sizeof(WARE));
		for(i=0;i<W;i++)
		{     
			fscanf(f,"%d %d\n",&wares[i].R,&wares[i].C);
			wares[i].prodcnt=(int*)calloc(P,sizeof(int));
			for(j=0;j<P;j++)
			{
				fscanf(f,"%d",&wares[i].prodcnt[j]);
				wares[i].tot+=wares[i].prodcnt[j];
			} 
			fscanf(f,"\n"); 
		}
		fscanf(f,"\n"); 

		maxorders=MIN(MAXORDERS,W*4);   
		// orders
		fscanf(f,"%d\n",&O);
		orders=(ORDER*)calloc(O,sizeof(ORDER));
		for(i=0;i<O;i++)
		{
			fscanf(f,"%d %d\n",&orders[i].R,&orders[i].C);
			fscanf(f,"%d\n",&orders[i].tot);
			orders[i].ltot=orders[i].tot;
			orders[i].id=i;
			orders[i].prodcnt=(int*)calloc(P,sizeof(int));
			orders[i].recprodcnt=(int*)calloc(P,sizeof(int));
			for(j=0;j<orders[i].tot;j++)
			{
				int item;
				fscanf(f,"%d",&item);
				orders[i].prodcnt[item]++;
				orders[i].recprodcnt[item]++;
			}
			fscanf(f,"\n"); 
		}
		qsort(orders,O,sizeof(ORDER),order_compare_zero);    

		fscanf(f,"\n");
		drones=(DRONE*)calloc(D,sizeof(DRONE));
		dcmd=(Ints*)calloc(D,sizeof(Ints));
		for(i=0;i<D;i++)
		{
			int f;
			drones[i].action=action_none;
			drones[i].warehouseID=-1;
			for(f=0;f<maxorders;f++)
			{
				drones[i].orderID[f]=-1;
				drones[i].prodcnt[f]=(int*)calloc(P,sizeof(int));
			} 
			if(W==1) 
			drones[i].warehouseID=0;
			drones[i].R=wares[drones[i].warehouseID].R;
			drones[i].C=wares[drones[i].warehouseID].C;
			Ints_new(&dcmd[i]);
		} 
		fclose(f);

		_cost=(int**)calloc(W,sizeof(int*));
		for(i=0;i<W;i++) 
		_cost[i]=(int*)calloc(O,sizeof(int));
		return 1;
	}
	else
	return 0; 
}

void freedata()
{
	int i,j;
	for(i=0;i<D;i++)
	{
		for(j=0;j<maxorders;j++)
		free(drones[i].prodcnt[j]);
		Ints_delete(&dcmd[i]);
	} 
	free(dcmd); 
	free(drones);
	for(i=0;i<W;i++)
	{
		free(wares[i].prodcnt);
		free(_cost[i]);
	}
	free(wares); 
	free(_cost);
	for(i=0;i<O;i++)
	free(orders[i].prodcnt);  
	free(orders); 
	free(prods);
}

int writeoutput(const char*filename)
{
	FILE*f=fopen(filename,"wb");
	if(f)
	{
		int d,cmds=0;
		for(d=0;d<D;d++)
		cmds+=dcmd[d].cItems/5;
		fprintf(f,"%d\n",cmds);
		for(d=0;d<D;d++)
		{
			int i;
			for(i=0;i<dcmd[d].cItems;i+=5)
			fprintf(f,"%d %c %d %d %d\n",dcmd[d].Items[i],dcmd[d].Items[i+1],dcmd[d].Items[i+2],dcmd[d].Items[i+3],dcmd[d].Items[i+4]);
		} 
		fclose(f);
		return 1;
	}       
	else
	return 0;
}

// ---------------------------------------------------------------------------
// choose a drone for a product request (for a specific order, in a warehouse)
// ---------------------------------------------------------------------------
int addtodrone(int orderID,int productID,int warehouseID,int add,int act)
{
	int i,r,best=-1,besti=-1,bestused=-1;
	for(r=0;(r<2)&&(besti==-1);r++)
	for(i=0;i<D;i++)
	if((drones[i].action==action_none)||(drones[i].action==action_loaded))
	if(
			((r==0)&&((drones[i].warehouseID==warehouseID)||((drones[i].R==wares[warehouseID].R)&&(drones[i].C==wares[warehouseID].C))))||
			((r==1)&&(drones[i].warehouseID==-1))
			)
	if(canaddordertodrone(&drones[i],warehouseID,orderID,r))
	{ 
		int used=0,score=0;
		while((drones[i].payload+prods[productID].weight*(used+1)<=MAXP)&&(used<add))
		used++;
		if(used)
		{
			int f,dist,distO;
			for(f=0;f<maxorders;f++)
			if(drones[i].orderID[f]==orderID)
			{
				score+=R*C;
				break;
			}
			score+=used+used*(add==used);
			dist=(R*C-movecost(drones[i].R,drones[i].C,wares[warehouseID].R,wares[warehouseID].C));
			distO=WO_movecost(warehouseID,orderID);        
			for(f=0;f<maxorders;f++)
			if(drones[i].orderID[f]!=-1)
			{
				int distM=movecost(orders[drones[i].orderID[f]].R,orders[drones[i].orderID[f]].R,orders[orderID].R,orders[orderID].C);
				if(distM<distO)
				distO=distM;
			}
			score+=dist+(R*C-distO);
			score-=drones[i].time/W;
			if(score>best)
			{
				best=score;
				besti=i; 
				bestused=used;
			} 
		}
	}  
	if(besti!=-1)      
	{
		int f,used=bestused,i=besti;
		if(act)
		{
			int time=movecost(drones[i].R,drones[i].C,wares[warehouseID].R,wares[warehouseID].C);
			if((drones[i].R==wares[warehouseID].R)&&(drones[i].C==wares[warehouseID].C))
			time=0;
			else
			{ 
				drones[i].time+=time;
				drones[i].R=wares[warehouseID].R;
				drones[i].C=wares[warehouseID].C;				
			}    
			drones[i].warehouseID=warehouseID;
			for(f=0;f<maxorders;f++)
			if((drones[i].orderID[f]==-1)||(drones[i].orderID[f]==orderID))
			{
				drones[i].orderID[f]=orderID;
				drones[i].prodcnt[f][productID]+=used;
				break;
			}  
			drones[i].payload+=prods[productID].weight*used;      
			drones[i].action=action_loaded;
			
			drones[i].time+=addcmd_load(i,warehouseID,productID,used);
		} 
		return used;
	} 

	return 0;  
}

// ---------------------------------------------------------------------------
// ask to fullfill a product request for a specific order
// ---------------------------------------------------------------------------
int addrequest(int orderID,int productID,int hm,int bestW,int act)
{
	int w,rused=0,used,wx,val,weight;
	int*wid=(int*)calloc(W,sizeof(int)*2);
	for(wx=w=0;(w<W)&&hm;w++)
	if(val=match(&orders[orderID],&wares[w],NULL,&weight))//wares[w].prodcnt[productID])
	{			 
		wid[wx*2]=w;				
		wid[wx*2+1]=WO_movecost(w,orderID)+WD_nearestcost(w,prods[productID].weight)+val*val;
		wx++;
	}
	if(wx>1)
	qsort(wid,wx,sizeof(int)*2,wid_compare); 
	for(w=0;(w<wx)&&hm;w++)
	{
		int ww=wid[w*2];
		while(1)
		{
			if(used=addtodrone(orderID,productID,ww,MIN(wares[ww].prodcnt[productID],hm),act))
			{
				if(act)
				{
					wares[ww].prodcnt[productID]-=used;
					wares[ww].tot-=used;
					orders[orderID].prodcnt[productID]-=used;
					orders[orderID].ltot-=used;					  
				} 
				hm-=used; 
				rused+=used;
			}				  
			if(used&&hm)
			continue;
			else
			break;  
		}  
	}					
	free(wid);
	return rused;   
}

// ---------------------------------------------------------------------------
// process drone delivery
// ---------------------------------------------------------------------------
int dronedeliver(int droneID)
{
	DRONE*drone=&drones[droneID];
	int   f,done=0,got=0;
	int  *wid=(int*)calloc(maxorders,sizeof(int)*2),w,wx;
	while(1)
	{
		for(wx=f=0;f<maxorders;f++)
		if((drone->orderID[f]!=-1)&&(orders[drone->orderID[f]].tot))
		{
			wid[wx*2]=f;
			wid[wx*2+1]=movecost(drone->R,drone->C,orders[drone->orderID[f]].R,orders[drone->orderID[f]].C);		 	  
			wx++;
		}
		
		if(wx)	
		{
			if(wx>1)	
			qsort(wid,wx,sizeof(int)*2,wid_compare); 
			for(w=0;w<wx;w++)
			{
				f=wid[w*2];
				if((drone->orderID[f]!=-1)&&(orders[drone->orderID[f]].tot))
				{		 
					int   i,time=movecost(drone->R,drone->C,orders[drone->orderID[f]].R,orders[drone->orderID[f]].C);
					drone->time+=time; 
					for(i=0;i<P;i++)
					if(orders[drone->orderID[f]].recprodcnt[i])
					if(drone->prodcnt[f][i])
					{
						int deliver=MIN(orders[drone->orderID[f]].recprodcnt[i],drone->prodcnt[f][i]);
						orders[drone->orderID[f]].recprodcnt[i]-=deliver;
						orders[drone->orderID[f]].tot-=deliver;
						drone->prodcnt[f][i]-=deliver;
						drone->payload-=deliver*prods[i].weight;
						drone->time+=1; 
						addcmd_deliver(droneID,orders[drone->orderID[f]].id,i,deliver);
					}
					drone->R=orders[drone->orderID[f]].R;
					drone->C=orders[drone->orderID[f]].C;				     
					drone->action=action_none; 		 				     
					orders[drone->orderID[f]].time=MAX(orders[drone->orderID[f]].time,drone->time);
					if(orders[drone->orderID[f]].tot==0)    
					ordercomplete(drone,drone->orderID[f]);
					drone->orderID[f]=drone->warehouseID=-1; 				     
					done++;
					if(f)
					got++;
				} 
			} 
		}  
		else
		break; 
	}  
	free(wid);	
	return done;
}

// ---------------------------------------------------------------------------
// choose the best order to process
// ---------------------------------------------------------------------------
int getbestorder(char*already,int*isbest,int way,int queue,int maxdistratio)
{
	int d,i,o,besto=-1,bestA=0x7FFFFFFF,best__=-1,notavail=0,minp=0;
	int*t;

	for(o=0;o<O;o++)
	if(already[o]==0)
	for(i=0;i<P;i++)
	if(orders[o].prodcnt[i])
	{
		if(prods[i].weight>minp)
		minp=prods[i].weight;
	}  

	if(way==0)
	{          
		for(d=0;(d<D);d++)
		if((drones[d].action==action_stop)||(drones[d].payload+minp>MAXP))
		notavail++;

		if(notavail>=MAX(maxorders,3))
		return -1;    
	}  

	t=(int*)calloc(W,sizeof(int));
	for(i=0;i<W;i++)
	t[i]=WD_nearestcost(i,minp);
	for(o=0;o<O;o++)
	if((already[o]==0)&&(orders[o].ltot))
	{
		int i,costA=0x7FFFFFFF,best_=-1;
		ORDER*A=&orders[o];
		if(way==0)
		{
			bestA=0;
			besto=o;
			break;
		}				
		for(i=0;i<W;i++)
		{   
			int   val,weight,cnt;						
			if(val=match(A,&wares[i],&cnt,&weight))
			{
				int cost,best=0;
				if(queue==1)
				cost=WO_movecost(i,o)+cnt;
				else
				cost=(R*C-WO_movecost(i,o))+cnt;
				if(t[i]) 
				if(queue==1)
				cost+=t[i];								
				else 
				cost+=(R*C-t[i]);								
				if(queue==1)
				if((val==A->ltot)&&(t[i]==0))
				{
					cost=cost/(cnt+weight);
					best=i; 
				} 
				else
				cost+=weight;
				else		
				cost+=weight;
				
				cost-=sqrt((double)orders[o].time); 								
				
				if(cost<costA)
				{
					costA=cost;
					best_=best;
				} 
			}  
		}   
		if(costA<bestA)
		{
			bestA=costA;
			best__=best_;
			besto=o;
		}
	}
	free(t);
	if(isbest)
	*isbest=best__;

	if(maxdistratio&&(bestA>R*C/(maxdistratio*maxdistratio)))
	return -1;
	return besto;  
}

// ---------------------------------------------------------------------------
// [Delivery] main loop
// ---------------------------------------------------------------------------

int execute(const char*input,const char*output)
{
	int  d,j,t=0,activedrones=0,activeorders=0,way=0,round=0,maxdistratio,emit; 
	char*already;
	int *prdw;

	if(!readinput(input))
	{
		printf("can't read %s\n",input);
		return -1;   
	}

	printf("%s [%d x %d]\n",input,R,C); 
	printf("%d warehouses - %d products - %d orders - %d drones (max p: %d)\n",W,P,O,D,MAXP); 

	already=(char*)calloc(O,sizeof(char));

	prdw=(int*)calloc(P,sizeof(int)*2); 
	for(j=0;j<P;j++)
	{
		prdw[j*2]=j;
		prdw[j*2+1]=prods[j].weight*prods[j].weight;
	}
	qsort(prdw,P,sizeof(int)*2,wid_compare_rev);       
	maxdistratio=0;
	while((activedrones=checkdronetimes())&&(activeorders=checkordertoprocess()))
	{
		int sum=0,o=0;
		printf("active drones: %d - active orders: %d/%d [score: %d]  \r",activedrones,activeorders,O,SCORE);  			  

		if(round==1)
		{
			if(W>1) way=1;
			maxdistratio=MAX(R,C)/4;
		}

		memset(already,0,O);  
		emit=0;
		while(1)
		{
			int best,o=getbestorder(already,&best,way,1,maxdistratio);
			if(o==-1)
			break;            
			else
			{ 
				ORDER*ord=&orders[o];
				int   j,sum=0;       
				for(j=0;j<P;j++)
				if(ord->prodcnt[prdw[j*2]])
				sum+=addrequest(o,prdw[j*2],ord->prodcnt[prdw[j*2]],best,1);                
				already[o]=1; 
				if(sum==0)
				break;
				emit++;         
			} 
		}			  		
		if(emit==0)
		maxdistratio--; 
		else 
		maxdistratio++;
		
		for(sum=d=0;d<D;d++)
		if(drones[d].action==action_loaded)
		sum+=dronedeliver(d);   
		
		round++;  						
	}			 
	free(prdw); 
	free(already);  
	printf("active drones: %d - active orders: %d/%d [score: %d]  \n\n",activedrones,activeorders,O,SCORE);  			   

	if(!writeoutput(output))
	{
		printf("can't write %s\n",output);
		return -2;   
	}  

	freedata();
	return SCORE;
}

// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	printf("#hashcode 2016 - Delivert\n");
	printf("Modena Team - Qualification Round\n\n");

#ifdef _DEBUG      
	{
		int score=execute("mother_of_all_warehouses.in","mother_of_all_warehouses.out");     
		score+=execute("busy_day.in","busy_day.out");     
		score+=execute("redundancy.in","redundancy.out");       
		printf("Global score: %d\n",score);
		char s[64];
		gets(s);
	} 
#endif

	if(argc>1)
	{
		return execute(argv[1],argv[2]);    
	}
	else
	{
		printf("Usage: exe <file.in> <file.out>\n");
		return -3;   
	} 
}

// ---------------------------------------------------------------------------

