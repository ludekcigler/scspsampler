% Load generated samples for intel extra vars data
% Compute evaluation of these data

Z = intel_eval_extravars(data);

% Compute Euclidean distance matrix between data
distance = pdist(data);

2d_scale = mds2(distance, 2);
