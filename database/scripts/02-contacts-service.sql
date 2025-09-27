CREATE TYPE contact_status AS ENUM ('active', 'blocked');
CREATE TYPE contact_request_status AS ENUM ('pending', 'accepted', 'rejected');

CREATE TABLE contacts (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    contact_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    status contact_status DEFAULT 'active',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT contacts_no_self_contact CHECK (user_id != contact_id),
    CONSTRAINT contacts_unique_pair UNIQUE (user_id, contact_id)
);

CREATE TABLE contact_requests (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    requester_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    requested_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    status contact_request_status DEFAULT 'pending',
    message TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT contact_requests_no_self_request CHECK (requester_id != requested_id),
    CONSTRAINT contact_requests_unique_pair UNIQUE (requester_id, requested_id)
);

CREATE INDEX idx_contacts_user_id ON contacts(user_id);
CREATE INDEX idx_contacts_contact_id ON contacts(contact_id);
CREATE INDEX idx_contacts_status ON contacts(status);

CREATE INDEX idx_contact_requests_requester_id ON contact_requests(requester_id);
CREATE INDEX idx_contact_requests_requested_id ON contact_requests(requested_id);
CREATE INDEX idx_contact_requests_status ON contact_requests(status);

CREATE TRIGGER update_contacts_updated_at BEFORE UPDATE ON contacts 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_contact_requests_updated_at BEFORE UPDATE ON contact_requests 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();