close all
clear

ls = getLebedevSphere(50);
lsxyz(:,1) = ls.x;
lsxyz(:,2) = ls.y;
lsxyz(:,3) = ls.z;
scatter3(ls.x,ls.y,ls.z)

% ls = getLebedevSphere(50); % 50, ...
% ls50leb(:,1) = ls.x;
% ls50leb(:,2) = ls.y;
% ls50leb(:,3) = ls.z;
% 
% ls = getLebedevSphere(86); % 50, ...
% ls86leb(:,1) = ls.x;
% ls86leb(:,2) = ls.y;
% ls86leb(:,3) = ls.z;
% 
% lsxyz = unique([ls50leb; ls86leb],'rows');


[speakerAzEl(:,1), speakerAzEl(:,2), ~] = cart2sph(lsxyz(:,1),lsxyz(:,2),lsxyz(:,3));
speakerAzEl = rad2deg(speakerAzEl);

[tf, index]=ismember(speakerAzEl,[0 -90],'rows');
speakerAzEl(tf,:) = [180 -90];

speakerAzEl = sortrows(speakerAzEl, [1 2]);

idx = find(speakerAzEl(:,1) == 0,1,'first');
speakerAzEl = [speakerAzEl(idx:end,:); speakerAzEl(1:idx-1,:)];

headers = {'ID','spkAz','spkEl','spkDist','angAcc','angPrec','distAcc'};
width = [45, 65, 65, 65, 65, 65, 65]; % IR-cap column width

speakerDist = 1.5; % loudspeaker - head distance (m)
angAcc = 1.0; % measurement angle max deviation (accuracy) (deg)
angPrec = 1.0; % measurement angle max deviation (precision) (deg)
distAcc = 0.1; % distance deviation (m)

%% SAVE CONFIG FILE
fileID = fopen('speaker_angles.xml', 'w');
fprintf(fileID,'<TABLE_DATA>\n');
fprintf(fileID,'    <HEADERS>\n');
for i = 1:length(headers)
    fprintf(fileID,'        <COLUMN columnId="%.0f" name="%s" width="%.0f"/>\n', i, string(headers(i)), width(i));
end
fprintf(fileID,'    </HEADERS>\n');
fprintf(fileID,'    <DATA>\n');
for i = 1:length(speakerAzEl)
%     params = sprintf('%s="%02d"', string(headers(1)), i);
    params = sprintf('%s="%.0f"', string(headers(1)), i);
    params = [params ' ' sprintf('%s="%.2f"', string(headers(2)), speakerAzEl(i,1))];
    params = [params ' ' sprintf('%s="%.2f"', string(headers(3)), speakerAzEl(i,2))];
    params = [params ' ' sprintf('%s="%.2f"', string(headers(4)), speakerDist)];
    params = [params ' ' sprintf('%s="%.2f"', string(headers(5)), angAcc)];
    params = [params ' ' sprintf('%s="%.2f"', string(headers(6)), angPrec)];
    params = [params ' ' sprintf('%s="%.2f"', string(headers(7)), distAcc)];
    fprintf(fileID,['        <ITEM ' params '/>\n']);
end
fprintf(fileID,'    </DATA>\n');
fprintf(fileID,'</TABLE_DATA>\n');
fclose(fileID);