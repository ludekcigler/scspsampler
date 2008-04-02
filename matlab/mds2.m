function [points,vaf]=mds2(distance,dimensions,metric)

% MDS multidimensional scaling, version 2 (michael.d.lee@dsto.defence.gov.au)
% [points,vaf]=mds(distance,dimensions,metric)
% 
% DISTANCE is an NxN symmetric matrix of pairwise distances or proximities (required)
% DIMENSIONS specifies the required dimensionality of the coordinate representation (required)
% METRIC specifies the Minkowski distance metric operating in the space (default=2)
%
% POINTS returns an NxDIMENSIONS matrix giving the derived coordinate locations
% VAF return the variance of the distance values accounted for by the solution

global A flatd r;

% check the number of arguments
error(nargchk(2,3,nargin));

% check the distance matrix
[n check]=size(distance);
if check~=n
   error('distance matrix must be square');
end;
if ~isequal(distance,distance')
   error('distance matrix must be symmetric');
end;

% check the number of dimensions
if (dimensions<1)|(dimensions~=round(dimensions))
   error('number of dimensions must be a positive integer');
end;

% set default arguments as necessary
if nargin<3, metric=2; end;

% metric is expected to be a positive number
% but this constraints is not explicitly imposed

% assign shorter argument names
d=distance;
dim=dimensions;
r=metric;

% normalise distances to lie between 0 and 1
reshift=min(min(d));
d=d-reshift;
rescale=max(max(d));
d=d/rescale;

% calculate the variance of  the distance matrix
dbar=(sum(sum(d))-trace(d))/n/(n-1);
temp=(d-dbar*ones(n)).^2;
vard=.5*(sum(sum(temp))-trace(temp));

% initialise variables
options=zeros(18,1);
options(1)=-1; %display
options(2)=1e-1; %x precision
options(3)=1e-1; %residuals precision
npairs=round(n*(n-1)/2);

% setup differences between coordinates
A=zeros(npairs,n);
cc=1;
for i=1:n-1
   for j=i+1:n
      A(cc,i)=1;
      A(cc,j)=-1;
      cc=cc+1;
   end;
end;

% setup target distances as vector
flatd=zeros(npairs,1);
cc=1;
for i=1:n-1
   for j=i+1:n
      flatd(cc)=d(i,j);
      cc=cc+1;
   end;
end;

% minimise the residual function and find residual
p0=rand(n,dim)*.05;
p=leastsq('mdsresiduals',p0,options);
resid=mdsresiduals(p);

% return the variance accounted for
% and coordinate location of the final solution
sse=sum(sum(resid.^2));
vaf=1-sse/vard;
points=p*rescale+reshift;
