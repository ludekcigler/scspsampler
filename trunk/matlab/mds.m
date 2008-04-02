function [points,vaf]=mds(distance,dimensions,metric,iterations,learnrate)

% MDS multidimensional scaling (michael.d.lee@dsto.defence.gov.au)
% [points,vaf]=mds(distance,dimensions,metric,iterations,learnrate)
% 
% DISTANCE is an NxN symmetric matrix of pairwise distances or proximities (required)
% DIMENSIONS specifies the required dimensionality of the coordinate representation (required)
% METRIC specifies the Minkowski distance metric operating in the space (default=2)
% ITERATIONS specifies the number of optimisation iterations performed (default=50)
% LEARNRATE specifies the learning rate used in optimisation (default=0.05)
%
% POINTS returns an NxDIMENSIONS matrix giving the derived coordinate locations
% VAF returns the variance of the distance values accounted for by the solution

% check the number of arguments
error(nargchk(2,5,nargin));

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
if nargin<5, iterations=50; end;
if nargin<4, learnrate=0.05; end;
if nargin<3, metric=2; end;

% check the number of iterations
if (iterations<1)|(iterations~=round(iterations))
   error('number of iterations must be a positive integer');
end;

% metric and learnrate are expected to be positive numbers
% but these constraints are not explicitly imposed

% assign shorter argument names
d=distance;
dim=dimensions;
r=metric;
lr=learnrate;

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
its=0;
p=rand(n,dim)*.01-.005;
dh=zeros(n);

% main loop for the given number of iterations
while (its<iterations)
   its=its+1;
   % randomly permute the objects to determine the order
   % in which they are pinned for this iteration
   pinning_order=randperm(n);
   for i=1:n
      m=pinning_order(i);
      % having pinned an object, move all of the other on each dimension
      % according to the learning rule
      for j=1:n
         if ~(m==j)
            dh(m,j)=norm(p(m,:)-p(j,:),r);
            dh(j,m)=dh(m,j);
            for k=1:dim
               p(j,k)=p(j,k)-lr*(dh(m,j)-d(m,j))*dh(m,j)^(1-r)*abs(p(j,k)-p(m,k))^(r-1)*sign(p(j,k)-p(m,k));
            end;
         end;
      end;
   end;
end;

% calculate the variance of  the distance matrix
dbar=(sum(sum(d))-trace(d))/n/(n-1);
temp=(d-dbar*ones(n)).^2;
vard=.5*(sum(sum(temp))-trace(temp));

% return the sum-squared error, variance accounted for
% and coordinate location of the final solution
sse=sum(sum((d-dh).^2))-trace((d-dh).^2);
vaf=1-sse/vard;
points=p*rescale+reshift;
